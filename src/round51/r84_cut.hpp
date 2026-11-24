#pragma once
// Round 8.1: small truth-table cut resynthesis under case-specific weighted
// AND2/OR2/NOT area and required-time constraints. Unconditional Boolean
// equivalence of alternatives is proven over independent cut-leaf variables;
// only complete live-DAG improvements are permitted by the caller.
#include "typed_netgraph.hpp"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <functional>
#include <limits>
#include <numeric>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
namespace r84cut {
using r51::Graph;using r51::Builder;using r51::Costs;using Clock=std::chrono::steady_clock;
struct Stats {
  uint64_t eligible_roots=0,visited_roots=0,cuts=0,verified_functions=0;
  uint64_t sop_candidates=0,shannon_candidates=0,selected=0,materializations=0;
  uint64_t timing_reject=0,invalid_cut=0,zero_gain_choices=0;
  bool budget_hit=false,work_limit_hit=false;double elapsed_ms=0;
  uint64_t ranked_topq_visited=0;
};
struct Result {Graph graph;Stats stats;};
inline int cost(const Costs&c,int op){return op==0?c.aa:op==1?c.ao:c.an;}
inline int delay(const Costs&c,int op){return op==0?c.da:op==1?c.dor:c.dn;}
inline std::vector<int> unite(const std::vector<int>&a,const std::vector<int>&b){
  std::vector<int>v;v.reserve(a.size()+b.size());
  std::set_union(a.begin(),a.end(),b.begin(),b.end(),std::back_inserter(v));return v;
}
struct Env {
  Builder&b;const Costs&c;const std::vector<int>&leaves;
  int k;
  bool isone(int v)const{return v>b.graph.n&&b.graph.nodes[v].op==2&&b.graph.nodes[v].a==0;}
  int neg(int a){
    if(a==0)return b.add(2,0,0,c);
    if(isone(a))return 0;
    if(a>b.graph.n&&b.graph.nodes[a].op==2)return b.graph.nodes[a].a;
    return b.add(2,a,0,c);
  }
  bool inverse(int a,int z)const{
    return (a>b.graph.n&&b.graph.nodes[a].op==2&&b.graph.nodes[a].a==z)||
           (z>b.graph.n&&b.graph.nodes[z].op==2&&b.graph.nodes[z].a==a);
  }
  int bin(int op,int a,int z){
    if(a==z)return a;
    if(inverse(a,z))return op==0?0:neg(0);
    if(op==0){if(a==0||z==0)return 0;if(isone(a))return z;if(isone(z))return a;}
    else{if(a==0)return z;if(z==0)return a;if(isone(a)||isone(z))return neg(0);}
    return b.add(op,a,z,c);
  }
  int combine(std::vector<int>terms,int op){
    if(terms.empty())return op==0?neg(0):0;
    std::sort(terms.begin(),terms.end(),[&](int a,int z){return b.ready[a]==b.ready[z]?a<z:b.ready[a]<b.ready[z];});
    while(terms.size()>1){
      int a=terms[0],z=terms[1];terms.erase(terms.begin(),terms.begin()+2);
      int x=bin(op,a,z);
      auto it=std::upper_bound(terms.begin(),terms.end(),x,[&](int v,int y){
        return b.ready[v]==b.ready[y]?v<y:b.ready[v]<b.ready[y];});
      terms.insert(it,x);
    }
    return terms.front();
  }
  int lit(int i,int sign){return sign?leaves.at(i):neg(leaves.at(i));}
  struct Cube {int fixed,val,cover,lits;};
  int sop(unsigned truth,int policy){
    const int assignments=1<<k,full=(1<<assignments)-1;
    truth&=unsigned(full);
    if(!truth)return 0;
    if(int(truth)==full)return neg(0);
    std::vector<Cube>prime;
    for(int fixed=0;fixed<(1<<k);++fixed)for(int val=0;val<(1<<k);++val){
      if(val&~fixed)continue;
      int bits=0;
      for(int x=0;x<assignments;++x)if((x&fixed)==val)bits|=1<<x;
      if((unsigned(bits)&~truth)!=0)continue;
      bool bigger=false;
      for(int j=0;j<k;++j)if(fixed&(1<<j)){
        int f=fixed^(1<<j),v=val&f,covers=0;
        for(int x=0;x<assignments;++x)if((x&f)==v)covers|=1<<x;
        if((unsigned(covers)&~truth)==0){bigger=true;break;}
      }
      if(!bigger)prime.push_back({fixed,val,bits,__builtin_popcount((unsigned)fixed)});
    }
    int uncovered=int(truth);std::vector<int>terms;
    while(uncovered){
      int best=-1;double bestscore=-1;
      for(int i=0;i<int(prime.size());++i){
        int cnt=__builtin_popcount((unsigned)(prime[i].cover&uncovered));
        if(!cnt)continue;
        double div= policy==0?(0.9+double(prime[i].lits)*double(c.aa)):
                        (0.9+double(prime[i].lits)*double(c.aa+c.an));
        double s=cnt/div;
        if(s>bestscore){bestscore=s;best=i;}
      }
      if(best<0)throw std::runtime_error("invalid SOP cover");
      auto q=prime[best];std::vector<int>l;
      for(int bit=0;bit<k;bit++)if(q.fixed&(1<<bit))l.push_back(lit(bit,(q.val>>bit)&1));
      terms.push_back(combine(std::move(l),0));
      uncovered&=~q.cover;
    }
    return combine(std::move(terms),1);
  }
  int build_shannon(unsigned truth,const std::vector<int>&order){
    const int assignments=1<<k;
    std::function<int(int,int,int)>work=[&](int depth,int knownmask,int knownval)->int{
      bool has0=false,has1=false;
      for(int s=0;s<assignments;++s)if((s&knownmask)==knownval){
        if(truth&(1u<<s))has1=true;else has0=true;
        if(has0&&has1)break;
      }
      if(!has1)return 0;
      if(!has0)return neg(0);
      if(depth>=k)throw std::runtime_error("shannon recursion invalid");
      int v=order.at(depth),bit=1<<v;
      int lo=work(depth+1,knownmask|bit,knownval);
      int hi=work(depth+1,knownmask|bit,knownval|bit);
      if(lo==hi)return lo;
      int x=leaves.at(v);
      if(lo==0)return bin(0,x,hi);
      if(hi==0)return bin(0,neg(x),lo);
      if(hi==b.add(2,0,0,c))return bin(1,x,lo);
      if(lo==b.add(2,0,0,c))return bin(1,neg(x),hi);
      return bin(1,bin(0,neg(x),lo),bin(0,x,hi));
    };
    return work(0,0,0);
  }
};
inline int truth(const Graph&g,int root,const std::vector<int>&leaves){
  int k=int(leaves.size());if(k<1||k>4)throw std::runtime_error("invalid cut size");
  const int assignments=1<<k,full=(1<<assignments)-1;
  std::unordered_map<int,int>cache;cache.reserve(128);
  std::function<int(int)>calculate=[&](int id)->int{
    if(auto it=cache.find(id);it!=cache.end())return it->second;
    if(id==0)return cache[id]=0;
    auto it=std::lower_bound(leaves.begin(),leaves.end(),id);
    if(it!=leaves.end()&&*it==id){
      int var=it-leaves.begin(),bits=0;
      for(int s=0;s<assignments;++s)if(s&(1<<var))bits|=1<<s;
      return cache[id]=bits;
    }
    if(id<=g.n)throw std::runtime_error("cut does not block all paths");
    auto x=g.nodes[id];int a=calculate(x.a);
    if(x.op==2)return cache[id]=full^a;
    int z=calculate(x.b);
    return cache[id]=x.op==0?a&z:a|z;
  };
  return calculate(root);
}
inline int cone_cost(const Graph&graph,int root,const std::vector<int>&leaves,const Costs&c){
  std::set<int>stop(leaves.begin(),leaves.end());
  std::set<int>seen;std::vector<int>stack{root};int value=0;
  while(!stack.empty()){
    int id=stack.back();stack.pop_back();
    if(id<=graph.n||stop.count(id)||!seen.insert(id).second)continue;
    if(int(seen.size())>80)return std::numeric_limits<int>::max()/3;
    auto x=graph.nodes[id];value+=cost(c,x.op);stack.push_back(x.a);
    if(x.op!=2)stack.push_back(x.b);
  }
  return value;
}
inline int private_reclaim(const Graph&g,int root,const std::vector<int>&leaves,
                           const std::vector<int>&uses,const Costs&c){
  std::set<int>stop(leaves.begin(),leaves.end());
  std::set<int>seen;std::vector<int>stack{root};int a=0;
  while(!stack.empty()){
    int v=stack.back();stack.pop_back();
    if(v<=g.n||stop.count(v)||!seen.insert(v).second)continue;
    if(v!=root && uses[v]>1)continue; // multi-fanout nodes are not reclaimable
    if(int(seen.size())>80)return 0;
    auto x=g.nodes[v];a+=cost(c,x.op);stack.push_back(x.a);
    if(x.op!=2)stack.push_back(x.b);
  }
  return a;
}
inline Result rewrite(const Graph&g,const Costs&c,int target,
                      const std::string&mode,double budget_ms=100.0,int maxroots=2500,
                      const std::string&sampling="stride",int work_roots=0){
  auto started=Clock::now(),deadline=started+std::chrono::microseconds((long long)(budget_ms*1000.0));
  Result out;out.graph=g;if(!g.valid)return out;
  const int sz=int(g.nodes.size()),K=4;
  bool sop_enabled=mode!="shannon", shannon_enabled=mode!="sop";
  bool choice_mode=mode=="choices";
  std::vector<int>use(sz,0),needed(sz,std::numeric_limits<int>::max()/4);
  for(int id=g.n+1;id<sz;++id){auto x=g.nodes[id];use[x.a]++;if(x.op!=2)use[x.b]++;}
  for(int o:g.outputs){use[o]++;needed[o]=std::min(needed[o],target);}
  for(int id=sz-1;id>g.n;--id){
    if(needed[id]>=std::numeric_limits<int>::max()/4)continue;
    auto x=g.nodes[id];int rem=needed[id]-delay(c,x.op);
    needed[x.a]=std::min(needed[x.a],rem);
    if(x.op!=2)needed[x.b]=std::min(needed[x.b],rem);
  }
  // Deterministic root selection: stride reproduces Round8.1's sampling,
  // priority chooses likely reclaimable, live roots without introducing any
  // case-ID dependence. Selected IDs are visited in original topological order.
  std::vector<unsigned char>root_selected(sz,0),topq_flags(sz,0);
  std::vector<int>arrival(sz,0);
  for(int id=g.n+1;id<sz;++id){
    auto node=g.nodes[id];
    arrival[id]=std::max(arrival[node.a],node.op==2?0:arrival[node.b])+delay(c,node.op);
  }
  if(sampling=="priority" || sampling=="priority_topq" ||
     sampling=="priority_mixed33" || sampling=="priority_mixed66"){
    std::vector<std::pair<long long,int>>ranked;
    ranked.reserve(sz);
    for(int id=g.n+1;id<sz;++id){
      if(needed[id]>=std::numeric_limits<int>::max()/4 ||!use[id])continue;
      auto x=g.nodes[id];
      long long potential=cost(c,x.op)*16LL;
      // Privately owned one-hop predecessors are more likely reclaimable.
      if(x.a>g.n&&use[x.a]==1)potential+=8LL*cost(c,g.nodes[x.a].op);
      if(x.op!=2&&x.b>g.n&&use[x.b]==1)potential+=8LL*cost(c,g.nodes[x.b].op);
      // Extra slack permits more structural alternatives; bounded to avoid
      // suppressing high-area critical cones entirely.
      // Exact timing remains checked after candidate emission.
      potential+=std::min(8,std::max(0,needed[id]-arrival[id]));
      ranked.push_back({-potential,id});
    }
    std::sort(ranked.begin(),ranked.end());

    // Top-Q selection: select exactly the root-work quota before the
    // topological rewrite pass. This differs from the historical Top-1500
    // then first-900-in-topological-order truncation.
    const bool quota_selection=(sampling=="priority_topq" || sampling=="priority_mixed33" ||
                                sampling=="priority_mixed66") && work_roots>0;
    const int selected_limit=quota_selection?std::min(maxroots,work_roots):maxroots;
    if(sampling=="priority_mixed33" || sampling=="priority_mixed66"){
      // Preserve priority information while reserving quota for low-ID
      // roots in the Top-1500 pool. This is a deterministic stratification,
      // not an ad-hoc case-specific exception.
      const int q=std::min(selected_limit,int(ranked.size()));
      const int reserve=(sampling=="priority_mixed33"?q/3:(2*q)/3);
      int picked=0;
      for(int j=0;j<reserve;++j){root_selected[ranked[j].second]=1;++picked;}
      std::vector<int> pool;
      for(int j=0;j<std::min(maxroots,int(ranked.size()));++j)
        pool.push_back(ranked[j].second);
      std::sort(pool.begin(),pool.end());
      for(int id:pool)if(!root_selected[id] && picked<q){root_selected[id]=1;++picked;}
    }else for(int j=0;j<std::min(selected_limit,int(ranked.size()));++j)
      root_selected[ranked[j].second]=1;
    // Diagnostic ground truth independent of actual selection policy.
    for(int j=0;j<std::min(work_roots,int(ranked.size()));++j)
      topq_flags[ranked[j].second]=1;
  }else{
    const int stride=std::max(1,(sz-g.n+maxroots-1)/maxroots);
    for(int id=g.n+1;id<sz;++id)if((id-g.n)%stride==0)
      root_selected[id]=1;
  }
  // Bounded cut sets per node. Every cut is a true separating frontier.
  std::vector<std::vector<std::vector<int>>>cuts(sz);
  for(int id=0;id<=g.n;++id)cuts[id].push_back({id});
  Builder b(g);std::vector<int>mapped(sz,-1);
  std::iota(mapped.begin(),mapped.begin()+g.n+1,0);
  for(int id=g.n+1;id<sz;++id){
    auto x=g.nodes[id];
    std::vector<std::vector<int>>poss;
    if(x.op==2){poss=cuts[x.a];}
    else for(auto&a:cuts[x.a])for(auto&z:cuts[x.b]){
      auto u=unite(a,z);if(u.size()<=K)poss.push_back(std::move(u));
    }
    std::sort(poss.begin(),poss.end(),[](auto&a,auto&z){
      if(a.size()!=z.size())return a.size()>z.size();
      return a<z;
    });
    poss.erase(std::unique(poss.begin(),poss.end()),poss.end());
    cuts[id].push_back({id});
    // Retain at least one cut for each cardinality when available.
    std::set<int>seen_sizes;
    for(auto&u:poss)if(u.size()>=1&&u.size()<=K && !seen_sizes.count(int(u.size()))){
      cuts[id].push_back(u);seen_sizes.insert(int(u.size()));
    }
    for(auto&u:poss){
      if(cuts[id].size()>=9)break;
      if(u.size()<2||u.size()>K)continue;
      if(std::find(cuts[id].begin(),cuts[id].end(),u)==cuts[id].end())cuts[id].push_back(u);
    }
    int da=mapped[x.a],dz=x.op==2?0:mapped[x.b];
    mapped[id]=b.add(x.op,da,dz,c);
    if(!root_selected[id]||!use[id])continue;
    ++out.stats.eligible_roots;
    // Deterministic quota, checked only at the boundary between complete roots.
    // A time deadline remains as an emergency protection, NOT a progress target.
    if(work_roots>0 && out.stats.visited_roots>=uint64_t(work_roots)){
      out.stats.work_limit_hit=true;break;
    }
    if((out.stats.eligible_roots&31)==0 && Clock::now()>deadline){out.stats.budget_hit=true;break;}
    ++out.stats.visited_roots;
    if(topq_flags[id])++out.stats.ranked_topq_visited;
    int original=mapped[id],chosen=original,score=std::numeric_limits<int>::max();
    // Evaluate local expression area relative to the same mapped cut frontier.
    for(int ci=1;ci<int(cuts[id].size());++ci){
      const auto&lv=cuts[id][ci];int k=int(lv.size());
      if(k<2||k>4)continue;
      ++out.stats.cuts;
      int f;
      try{f=truth(g,id,lv);}catch(...){++out.stats.invalid_cut;continue;}
      ++out.stats.verified_functions;
      std::vector<int>actual;for(int v:lv)actual.push_back(mapped[v]);
      Env e{b,c,actual,k};
      int reclaim=private_reclaim(g,id,lv,use,c);
      if(reclaim<=0)continue;
      // Per-cut comparison; global acceptance is required by the caller.
      std::vector<int>local;
      if(sop_enabled){
        for(int policy=0;policy<2;++policy){
          try{
            local.push_back(e.sop((unsigned)f,policy));
            local.push_back(e.neg(e.sop((unsigned)((1<<(1<<k))-1)^f,policy)));
            out.stats.sop_candidates+=2;
          }catch(...){++out.stats.invalid_cut;}
        }
      }
      if(shannon_enabled){
        std::vector<int>ord(k);std::iota(ord.begin(),ord.end(),0);
        for(int i=0;i<std::min(k,4);++i){
          std::rotate(ord.begin(),ord.begin()+1,ord.end());
          try{local.push_back(e.build_shannon(f,ord));out.stats.shannon_candidates++;}
          catch(...){++out.stats.invalid_cut;}
        }
      }
      for(int candidate:local){
        ++out.stats.materializations;
        if(b.ready[candidate]>needed[id]){++out.stats.timing_reject;continue;}
        int candidate_cost=cone_cost(b.graph,candidate,actual,c);
        int diff=candidate_cost-reclaim;
        if((diff<0||(choice_mode&&diff==0)) &&
           (diff<score || (diff==score && b.ready[candidate]<b.ready[chosen]))){
          score=diff;chosen=candidate;
        }
      }
    }
    if(chosen!=original){
      if(score==0)++out.stats.zero_gain_choices;
      mapped[id]=chosen;++out.stats.selected;
    }
  }
  // If the budget expires, map all remaining nodes without searching.
  for(int id=g.n+1;id<sz;++id)if(mapped[id]<0){
    auto x=g.nodes[id];mapped[id]=b.add(x.op,mapped[x.a],x.op==2?0:mapped[x.b],c);
  }
  std::vector<int>outputs;for(int v:g.outputs)outputs.push_back(mapped[v]);
  out.graph=b.finish(std::move(outputs));
  out.stats.elapsed_ms=std::chrono::duration<double,std::milli>(Clock::now()-started).count();
  return out;
}
} // namespace r84cut
