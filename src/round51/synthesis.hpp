// Adapted from Jincan LI and Zeyun DU, Round 5.1 (see ORIGIN.md).
#pragma once
#ifndef ROUND5_CACHE
#define ROUND5_CACHE 1
#endif
#include "polarity_mapper.hpp"
#include "algebraic_mapper.hpp"
#include "typed_netgraph.hpp"
#include <bits/stdc++.h>
namespace round51 {
using namespace std;
struct BKey {int v,lo,hi;bool operator==(const BKey&o)const{return v==o.v && lo==o.lo && hi==o.hi;}};
struct H3 {size_t operator()(const BKey&k)const { uint64_t z=((uint64_t)(unsigned)k.v<<55)^((uint64_t)(unsigned)k.lo<<26)^(unsigned)k.hi; z^=z>>30;z*=0xbf58476d1ce4e5b9ULL;z^=z>>27;z*=0x94d049bb133111ebULL;return z^(z>>31);}};
struct BNode{int v,lo,hi;};
struct Opt {string order;bool fast, rewrite, expand;int cubemode,margin;bool affine=false;bool area_first=false;bool regular=false;bool decoder=false;};
struct CandidateTimeout {};

struct Truth {
    int n, m, aa, da, ao, doo, an, dn;
    const TruthTable& T;
    const vector<long long>& influence;
};
// Compile invariant Quine-McCluskey / greedy-cover choices once per
// truth-table invocation. Mapping to gates is still candidate-specific.
struct R5Cube {int val,mask;};
struct R5CoverPlan {int kind=-1; vector<int> support; vector<R5Cube> chosen;};
const std::vector<std::array<R5CoverPlan,2>>& shared_covers(const Truth &data, Clock::time_point deadline){
 static std::vector<std::array<R5CoverPlan,2>> cache;
 static const TruthTable* last_table=nullptr;
 if(last_table==&data.T)return cache;
 last_table=&data.T;
 cache.assign(data.m,{});
 if(data.n>13)return cache;
 const int n=data.n,m=data.m;
 const auto &T=data.T;
 for(int j=0;j<m;j++){
  vector<int> support;
  for(int v=0;v<n;v++)
   if(T.support[j] & (uint32_t(1)<<v))support.push_back(v);
  int k=(int)support.size();
  if(k>9)continue;
  int Q=1<<k;vector<uint8_t> tt(Q);
  for(int i=0;i<Q;i++){
   size_t orig=0;
   for(int b=0;b<k;b++)if(i&(1<<b))orig|=1ull<<(n-1-support[k-1-b]);
   tt[i]=T.bit(orig,j);
  }
  for(int goal=0;goal<=1;goal++){
   R5CoverPlan &plan=cache[j][goal];
   plan.support=support;
   vector<R5Cube> cur;cur.reserve(Q);
   for(int i=0;i<Q;i++)if(tt[i]==goal)cur.push_back({i,Q-1});
   if(cur.empty()){plan.kind=0;continue;}
   if((int)cur.size()==Q){plan.kind=1;continue;}
   plan.kind=2;
   vector<R5Cube> prime;
   while(!cur.empty()){
    vector<char> used(cur.size());vector<R5Cube> nxt;
    unordered_set<unsigned long long> hash;
    for(size_t a=0;a<cur.size();a++)for(size_t b=a+1;b<cur.size();b++){
      if (b == a+1 && Clock::now() >= deadline) throw CandidateTimeout{};
      if(cur[a].mask!=cur[b].mask)continue;
      int diff=(cur[a].val^cur[b].val)&cur[a].mask;
      if(__builtin_popcount((unsigned)diff)!=1)continue;
      used[a]=used[b]=1;
      int mk=cur[a].mask^diff,v=cur[a].val&mk;
      auto id=((unsigned long long)(unsigned)mk<<32)|(unsigned)v;
      if(hash.insert(id).second)nxt.push_back({v,mk});
    }
    for(size_t a=0;a<cur.size();a++)if(!used[a])prime.push_back(cur[a]);
    cur.swap(nxt);
   }
   sort(prime.begin(),prime.end(),[](R5Cube a,R5Cube b){return tie(a.mask,a.val)<tie(b.mask,b.val);});
   prime.erase(unique(prime.begin(),prime.end(),[](R5Cube a,R5Cube b){return a.mask==b.mask&&a.val==b.val;}),prime.end());
   vector<char> uncovered(Q,0);int remaining=0;
   for(int i=0;i<Q;i++)if(tt[i]==goal){uncovered[i]=1;remaining++;}
   while(remaining){
    double best=-1;int which=-1;
    for(int t=0;t<(int)prime.size();t++){
      int gains=0;
      for(int i=0;i<Q;i++)if(uncovered[i] && (i&prime[t].mask)==prime[t].val)gains++;
      if(!gains)continue;
      int cost=max(1,__builtin_popcount((unsigned)prime[t].mask));
      double score=double(gains)/(0.4+cost);
      if(score>best){best=score;which=t;}
    }
    if(which<0)throw runtime_error("cover failed");
    auto q=prime[which];plan.chosen.push_back(q);
    for(int i=0;i<Q;i++)if(uncovered[i] && (i&q.mask)==q.val){uncovered[i]=0;remaining--;}
   }
  }
 }
 return cache;
}
r51::Graph run_synth(const Truth &data,const Opt &opt,
  chrono::steady_clock::time_point deadline=chrono::steady_clock::time_point::max()){
 uint64_t steps=0;
 auto checkpoint=[&](){
   if(deadline!=chrono::steady_clock::time_point::max() &&
      ((++steps & 1023ULL)==0) && (chrono::steady_clock::now()>deadline || ((steps & 65535ULL)==0 && !search_memory.available())))
      throw CandidateTimeout{};
 };

 const int n=data.n,m=data.m;
 const auto &T=data.T;
 const int aa=data.aa,da=data.da,ao=data.ao,doo=data.doo,an=data.an,dn=data.dn;
 // Round 5: build a ROBDD once per identical variable ordering, and reuse
 // its immutable structure across mapping configurations. Only the repeated
 // influence-desc order is cached. A bounded node cap avoids holding large
 // BDDs while a different order is being constructed.
 using Time=std::chrono::steady_clock;
 const auto bdd_started=Time::now();
 struct FrozenGraph {
   vector<BNode> nodes;
   vector<int> roots;
   vector<uint64_t> af;
 };
 static std::shared_ptr<const FrozenGraph> shared_desc;

 std::shared_ptr<const FrozenGraph> graph;
 bool bdd_hit=false;
#if ROUND5_CACHE
 if(!opt.regular && opt.order=="influence-desc" && shared_desc) {
    graph=shared_desc;
    bdd_hit=true;
 }
#endif
 if(!graph) {
  size_t max_rows=1;
  for(uint32_t support:T.support)
   max_rows=max(max_rows,size_t(1)<<__builtin_popcount(support));
  const size_t node_capacity=min(max_rows,size_t(65536));
  const size_t bucket_count=min(max_rows*2,size_t(131072));
  const size_t scratch_bytes=max_rows*sizeof(int) + size_t(m)*sizeof(int)
                            + node_capacity*sizeof(BNode) + bucket_count*sizeof(void*);
  if(!search_memory.available(scratch_bytes))throw CandidateTimeout{};
  auto current=std::make_shared<FrozenGraph>();
  auto &nodes=current->nodes;
  vector<int> perm(n);
  auto &roots=current->roots;
  nodes.reserve(node_capacity);nodes.push_back({-1,0,0});
  unordered_map<BKey,int,H3> cache;
  cache.reserve(bucket_count);
  iota(perm.begin(),perm.end(),0);
  string order=opt.order;
  if(order=="reverse")reverse(perm.begin(),perm.end());
  if(order=="rotate")rotate(perm.begin(),perm.begin()+n/2,perm.end());
  if(order=="influence-asc"||order=="influence-desc"){
   const auto &influence=data.influence;
   stable_sort(perm.begin(),perm.end(),[&](int a,int b){return order=="influence-asc"?influence[a]<influence[b]:influence[a]>influence[b];});
  }
  roots.resize(m);
  vector<int> cur(max_rows);
  for(int j=0;j<m;j++){
   // Increment the assignment in this output's support order. A binary
   // carry flips the lowest bits through the first one in the new index.
   if(order=="output-influence"){
    vector<size_t> changes(n);
    for(int v=0;v<n;++v)
     changes[v]=T.changed_pairs(j,size_t(1)<<(n-1-v));
    iota(perm.begin(),perm.end(),0);
    stable_sort(perm.begin(),perm.end(),[&](int a,int b){return changes[a]>changes[b];});
   }
   vector<int> levels;
   vector<size_t> carry;
   size_t toggles=0;
   for(int v=n-1;v>=0;--v){
    if(!(T.support[j] & (uint32_t(1)<<perm[v])))continue;
    levels.push_back(perm[v]);
    toggles |= size_t(1)<<(n-1-perm[v]);
    carry.push_back(toggles);
   }
   size_t count=size_t(1)<<levels.size(), row=0;
   cur[0]=T.bit(0,j);
   for(size_t i=1;i<count;++i){
    checkpoint();
    row ^= carry[__builtin_ctzll(i)];
    cur[i]=T.bit(row,j);
   }
   for(int v:levels){
    // Each reduced pair can overwrite its first half after both children are read.
    for(size_t k=0;k<count/2;k++){
     checkpoint();int lo=cur[2*k],hi=cur[2*k+1];
     if(lo==hi){cur[k]=lo;continue;}
     int flip=opt.regular?0:(hi&1);int normlo=lo^flip,normhi=hi^flip;
     BKey key={v,normlo,normhi};auto it=cache.find(key);
     if(it!=cache.end()){cur[k]=it->second^flip;continue;}
     int id=nodes.size();
     if(deadline!=chrono::steady_clock::time_point::max() && id>600000)throw CandidateTimeout{};
     nodes.push_back({v,normlo,normhi});cache.emplace(key,id<<1);cur[k]=(id<<1)^flip;
    }
    count/=2;
   }
   roots[j]=cur[0];
  }
  // The affine table is a pure function of ROBDD structure and bit ordering.
  const uint64_t cb=(1ULL<<n), BAD=~0ULL;
  auto &af=current->af;
  if(!search_memory.available(nodes.size()*2*sizeof(uint64_t)))throw CandidateTimeout{};
  af.assign(nodes.size()*2,BAD);
  af[0]=0;af[1]=cb;
  for(size_t k=1;k<nodes.size();k++){
   const auto &nd=nodes[k];
   if(af[nd.lo]!=BAD && af[nd.hi]!=BAD && (af[nd.lo]^af[nd.hi])==cb){
     af[2*k]=af[nd.lo]^(1ULL<<nd.v);
     af[2*k+1]=af[2*k]^cb;
   }
  }
  graph=current;
#if ROUND5_CACHE
  if(!opt.regular && opt.order=="influence-desc" && n<=18 && nodes.size()<=80000){

    shared_desc=std::move(current);
  }
#endif
 }
 const auto &nodes=graph->nodes;
 const auto &roots=graph->roots;
 const uint64_t cb=(1ULL<<n), BAD=~0ULL;
 const auto &af=graph->af;
 const auto bdd_finished=Time::now();
 if(auto p=std::getenv("UFUG_ROUND5_PROFILE"))if(*p=='1') {
   std::cerr<<"R5_STAGE stage=bdd order="<<opt.order
      <<" cache_hit="<<bdd_hit<<" nodes="<<nodes.size()
      <<" elapsed_ms="<<std::fixed<<std::setprecision(3)
      <<std::chrono::duration<double,std::milli>(bdd_finished-bdd_started).count()<<"\n";
 }
 // Emit gates with structural sharing, constant folding, and NOT sharing.
 r51::Builder builder(r51::Graph(n,m));
 builder.intern.reserve(nodes.size()*2);
 const r51::Costs costs{aa,da,ao,doo,an,dn};
 const auto &gates=builder.graph.nodes;
 const auto &ready=builder.ready;
 auto mk=[&](int op,int a,int b){
  checkpoint();
  return builder.add(op,a,b,costs);
 };
 int ONE=mk(2,0,0); // canonical 1
 function<int(int)> neg=[&](int a)->int{
  if(a==0)return ONE;
  if(a==ONE)return 0;
  auto it=builder.intern.find({2,a,0});if(it!=builder.intern.end())return it->second;
  if(a>n){const auto &g=gates[a];if(g.op==2)return g.a;}
  return mk(2,a,0);
 };
 auto areInverse=[&](int a,int b)->bool {
   if((a==0&&b==ONE)||(a==ONE&&b==0))return true;
   if(a>n && gates[a].op==2 && gates[a].a==b)return true;
   if(b>n && gates[b].op==2 && gates[b].a==a)return true;
   return false;
 };
 function<int(int,int)> land=[&](int a,int b)->int{if(a==0||b==0)return 0;if(a==ONE)return b;if(b==ONE)return a;if(a==b)return a;if(areInverse(a,b))return 0;if((opt.rewrite && aa>ao+3*an) || opt.fast){
  int na=neg(a),nb=neg(b);
  if(opt.area_first || (opt.fast?max(ready[na],ready[nb])+doo+dn<max(ready[a],ready[b])+da:max(ready[na],ready[nb])+doo+dn<=max(ready[a],ready[b])+da))return neg(mk(1,na,nb));
  }return mk(0,a,b);};
 function<int(int,int)> lor=[&](int a,int b)->int{if(a==ONE||b==ONE)return ONE;if(a==0)return b;if(b==0)return a;if(a==b)return a;if(areInverse(a,b))return ONE;if((opt.rewrite && ao>aa+3*an) || opt.fast){
  int na=neg(a),nb=neg(b);
  if(opt.area_first || (opt.fast?max(ready[na],ready[nb])+da+dn<max(ready[a],ready[b])+doo:max(ready[na],ready[nb])+da+dn<=max(ready[a],ready[b])+doo))return neg(mk(0,na,nb));
  }return mk(1,a,b);};

 function<int(int,int)> makeXor=[&](int a,int b)->int{
   if(a==b)return 0;
   if(areInverse(a,b))return ONE;
   int p=land(lor(a,b),neg(land(a,b)));
   int q=lor(land(a,neg(b)),land(neg(a),b));
   if(opt.fast)return ready[p]<=ready[q]?p:q;
   return ready[p]<=ready[q]?p:q;
 };
 auto makeAffine=[&](uint64_t mask)->int{
   using Item=pair<int,int>;
   priority_queue<Item,vector<Item>,greater<Item>> pq;
   for(int v=0;v<n;v++)if(mask&(1ULL<<v))pq.emplace(0,v+1);
   while(pq.size()>1){int a=pq.top().second;pq.pop();int b=pq.top().second;pq.pop();int c=makeXor(a,b);pq.emplace(ready[c],c);}
   int root=pq.empty()?0:pq.top().second;
   return (mask&cb)?neg(root):root;
 };
 vector<int> done(nodes.size()*2,-1);done[0]=0;done[1]=ONE;
 function<int(int)> build=[&](int id)->int {
  checkpoint();if(done[id]>=0)return done[id];
  if(opt.affine && af[id]!=BAD)return done[id]=makeAffine(af[id]);
  if((id&1) && done[id^1]>=0)return done[id]=neg(done[id^1]);
  auto node=nodes[id>>1];int x=node.v+1;
  // Decode a pair of selectors once; equal cofactors share a selector mask.
  if(opt.decoder && node.lo>1 && node.hi>1) {
   int low=node.lo^(id&1), high=node.hi^(id&1);
   auto l=nodes[low>>1], h=nodes[high>>1];
   if(l.v==h.v) {
    const int y=l.v+1;
    const int leaf[]={l.lo^(low&1),l.hi^(low&1),h.lo^(high&1),h.hi^(high&1)};
    auto selector=[&](unsigned mask) {
     switch(mask) {
      case 15: return ONE;
      case 3: return neg(x);
      case 12: return x;
      case 5: return neg(y);
      case 10: return y;
      case 6: return makeXor(x,y);
      case 9: return neg(makeXor(x,y));
     }
     bool complement=__builtin_popcount(mask)==3;
     unsigned corner=__builtin_ctz(complement?(15U^mask):mask);
     int term=land((corner&2)?x:neg(x),(corner&1)?y:neg(y));
     return complement?neg(term):term;
    };
    int terms[4], count=0;
    unsigned used=0;
    for(int i=0;i<4;++i) {
     if(!leaf[i] || (used&(1U<<i)))continue;
     unsigned mask=0;
     for(int j=i;j<4;++j)if(leaf[j]==leaf[i])mask|=1U<<j;
     used|=mask;
     terms[count++]=land(selector(mask),build(leaf[i]));
    }
    if(!count)return done[id]=0;
    while(count>1) {
     std::sort(terms,terms+count,[&](int a,int b){return ready[a]>ready[b];});
     terms[count-2]=lor(terms[count-2],terms[count-1]);
     --count;
    }
    return done[id]=terms[0];
   }
  }
  int lo=build(node.lo^(id&1)),hi=build(node.hi^(id&1)),result;
  if(lo==hi)result=lo;
  else if(lo==0)result=land(x,hi);
  else if(hi==0)result=land(neg(x),lo);
  else if(lo==ONE)result=lor(neg(x),hi);
  else if(hi==ONE)result=lor(x,lo);
  else result=lor(land(neg(x),lo),land(x,hi));
  // Experimental two-level Shannon expansion, compare by local arrival time.
  if(opt.expand && node.lo > 1 && node.hi > 1) {
    auto combine_balanced=[&](vector<int> v,bool isor)->int {
      if(v.empty())return isor?0:ONE;
      auto cmp=[&](int a,int b){return ready[a]>ready[b];};
      priority_queue<int,vector<int>,decltype(cmp)> pq(cmp);
      for(int a:v)pq.push(a);
      while(pq.size()>1){int a=pq.top();pq.pop();int b=pq.top();pq.pop();pq.push(isor?lor(a,b):land(a,b));}
      return pq.top();
    };
    vector<int> terms;
    for(int side=0;side<2;side++){
      int child=(side?node.hi:node.lo)^(id&1);
      int rootlit=side?x:neg(x);
      if(child <= 1) { if(child==1)terms.push_back(rootlit);continue; }
      auto g=nodes[child>>1];int y=g.v+1;
      for(int side2=0;side2<2;side2++){
        int leaf=build((side2?g.hi:g.lo)^(child&1));
        if(leaf==0)continue;
        vector<int> vs={rootlit,side2?y:neg(y)};
        if(leaf!=ONE)vs.push_back(leaf);
        terms.push_back(combine_balanced(vs,false));
      }
    }
    int expanded=combine_balanced(terms,true);
    int slack=opt.margin;
    if(ready[expanded]+slack < ready[result])result=expanded;
  }
  done[id]=result;return result;
 };
 vector<int> outs;for(int r:roots)outs.push_back(build(r));
 // Cached symbolic implicant covers. Their chosen terms depend only on
 // the truth table; gate construction, hashing and arrival times still depend
 // on this candidate's mapping strategy.
 int cubemode=opt.cubemode;
 if(cubemode && n<=13){
  const auto &covers=shared_covers(data,deadline);
  auto combine=[&](vector<int> vs,bool isOr)->int {
   if(vs.empty())return isOr?0:ONE;
   auto cmp=[&](int a,int b){return ready[a]>ready[b];};
   priority_queue<int,vector<int>,decltype(cmp)> pq(cmp);
   for(int a:vs)pq.push(a);
   while(pq.size()>1){int a=pq.top();pq.pop();int b=pq.top();pq.pop();pq.push(isOr?lor(a,b):land(a,b));}
   return pq.top();
  };
  auto calcSOP=[&](const R5CoverPlan &plan)->int{
   if(plan.kind<0)return -1;
   if(plan.kind==0)return 0;
   if(plan.kind==1)return ONE;
   const int k=(int)plan.support.size();
   vector<int> terms;
   for(auto p:plan.chosen){
     vector<int> literals;
     for(int b=0;b<k;b++)if(p.mask&(1<<b)){
       int x=plan.support[k-1-b]+1;
       literals.push_back((p.val&(1<<b))?x:neg(x));
     }
     terms.push_back(combine(literals,false));
   }
   return combine(terms,true);
  };
  for(int j=0;j<m;j++){
   int old=outs[j];
   int p=calcSOP(covers[j][1]);
   if(p<0)continue;
   int q=neg(calcSOP(covers[j][0]));
   int candidate=(ready[p]<=ready[q]?p:q);
   if(cubemode==2 || ready[candidate]<ready[old])outs[j]=candidate;
  }
 }

 if(auto p=std::getenv("UFUG_ROUND5_PROFILE"))if(*p=='1') {
   std::cerr<<"R5_STAGE stage=mapping order="<<opt.order
     <<" elapsed_ms="<<std::fixed<<std::setprecision(3)
     <<std::chrono::duration<double,std::milli>(Time::now()-bdd_finished).count()<<"\n";
 }
 return builder.finish(std::move(outs));
}


} // namespace round51
