#pragma once
// Round 5.1: typed, in-memory postprocessing; candidate semantics are unchanged.
#include <algorithm>
#include <cstdint>
#include <climits>
#include <numeric>
#include <queue>
#include <unordered_map>
#include <vector>

namespace r51 {
struct Node {int op,a,b;}; // 0=AND2, 1=OR2, 2=NOT, -1=constant/input
struct Costs {int aa,da,ao,dor,an,dn;};
struct Metric {bool valid=false; long long area=0;int delay=0;};
struct Key {int op,a,b;bool operator==(const Key& k)const{return op==k.op&&a==k.a&&b==k.b;}};
struct Hash {size_t operator()(const Key& k)const{
 uint64_t x=(uint32_t)k.op;
 x=x*0x9e3779b97f4a7c15ULL+(uint32_t)k.a;
 x=x*0xbf58476d1ce4e5b9ULL+(uint32_t)k.b;
 return (size_t)(x^(x>>32));
}};

struct Graph {
 int n=0,m=0;
 std::vector<Node> nodes;
 std::vector<int> outputs;
 bool valid=false;
 Graph()=default;
 Graph(int n_,int m_):n(n_),m(m_),nodes(n_+1,{-1,0,0}),valid(true){}
 Metric metric(const Costs& c)const{
  if(!valid || int(outputs.size())!=m || int(nodes.size())<n+1)return {};
  std::vector<int> ready(nodes.size(),0);
  long long area=0;
  for(int id=n+1;id<int(nodes.size());++id){
   const Node&g=nodes[id];
   if(g.op<0||g.op>2||g.a<0||g.a>=id ||(g.op!=2 && (g.b<0||g.b>=id)))return {};
   ready[id]=g.op==2?ready[g.a]+c.dn:std::max(ready[g.a],ready[g.b])+(g.op==0?c.da:c.dor);
   area+=g.op==0?c.aa:g.op==1?c.ao:c.an;
  }
  int delay=0;
  for(int v:outputs){if(v<0||v>=int(nodes.size()))return {};delay=std::max(delay,ready[v]);}
  return {true,area,delay};
 }
};

// On construction, each new gate gets its exact Arrival Time immediately.
// The Hash-consed representation is kept in memory until the candidate is chosen.
struct Builder {
 Graph graph;
 std::vector<int> ready;
 std::unordered_map<Key,int,Hash> intern;
 explicit Builder(const Graph& old):graph(old.n,old.m),ready(old.n+1,0){intern.reserve(old.nodes.size()*2);}
 int add(int op,int a,int b,const Costs&c){
  if(op!=2&&a>b)std::swap(a,b);
  Key key{op,a,b};auto it=intern.find(key);if(it!=intern.end())return it->second;
  int id=graph.nodes.size();graph.nodes.push_back({op,a,b});
  ready.push_back(op==2?ready[a]+c.dn:std::max(ready[a],ready[b])+(op==0?c.da:c.dor));
  intern.emplace(key,id);return id;
 }
 Graph finish(std::vector<int> outputs){
  auto &g=graph;
  std::vector<unsigned char> live(g.nodes.size());std::vector<int> st=outputs;
  while(!st.empty()){
   int v=st.back();st.pop_back();
   if(v<=g.n||live[v])continue;
   live[v]=1;st.push_back(g.nodes[v].a);
   if(g.nodes[v].op!=2)st.push_back(g.nodes[v].b);
  }
  Graph result(g.n,g.m);
  std::vector<int> ids(g.nodes.size());
  std::iota(ids.begin(),ids.begin()+g.n+1,0);
  for(int id=g.n+1;id<int(g.nodes.size());++id)if(live[id]){
   auto x=g.nodes[id];result.nodes.push_back({x.op,ids[x.a],x.op==2?0:ids[x.b]});
   ids[id]=int(result.nodes.size())-1;
  }
  for(int v:outputs)result.outputs.push_back(ids[v]);
  return result;
 }
};

inline Graph rebalance(const Graph &old,const Costs&c){
 if(!old.valid)return {};
 std::vector<int> uses(old.nodes.size(),0);
 for(int id=old.n+1;id<int(old.nodes.size());id++){
  ++uses[old.nodes[id].a];
  if(old.nodes[id].op!=2)++uses[old.nodes[id].b];
 }
 for(int o:old.outputs)++uses[o];
 Builder b(old);
 std::vector<int> mapped(old.nodes.size(),-1);
 std::iota(mapped.begin(),mapped.begin()+old.n+1,0);
 for(int id=old.n+1;id<int(old.nodes.size());id++){
  auto g=old.nodes[id];
  if(g.op==2){mapped[id]=b.add(2,mapped[g.a],0,c);continue;}
  std::vector<int> leaves;
  std::vector<int> st{id};
  while(!st.empty()){
    int v=st.back();st.pop_back();
    if(v>old.n && old.nodes[v].op==g.op && (v==id||uses[v]==1) && leaves.size()+st.size()<64){
      st.push_back(old.nodes[v].a);st.push_back(old.nodes[v].b);
    }else leaves.push_back(mapped[v]);
  }
  std::sort(leaves.begin(),leaves.end());leaves.erase(std::unique(leaves.begin(),leaves.end()),leaves.end());
  if(leaves.size()==1){mapped[id]=leaves[0];continue;}
  using Item=std::pair<int,int>;
  std::priority_queue<Item,std::vector<Item>,std::greater<Item>> pq;
  for(int v:leaves)pq.emplace(b.ready[v],v);
  while(pq.size()>1){
    int a=pq.top().second;pq.pop();int q=pq.top().second;pq.pop();
    int z=b.add(g.op,a,q,c);pq.emplace(b.ready[z],z);
  }
  mapped[id]=pq.top().second;
 }
 std::vector<int> outputs;
 for(int o:old.outputs)outputs.push_back(mapped[o]);
 return b.finish(std::move(outputs));
}

inline Graph area_recover(const Graph&old,const Costs&c,int target,int margin){
 if(!old.valid)return {};
 const int INF=1000000000;
 std::vector<int> need(old.nodes.size(),INF);
 for(int o:old.outputs)need[o]=std::min(need[o],target-margin);
 for(int id=int(old.nodes.size())-1;id>old.n;--id){
   if(need[id]>=INF)continue;
   auto g=old.nodes[id];int d=g.op==0?c.da:g.op==1?c.dor:c.dn;
   need[g.a]=std::min(need[g.a],need[id]-d);
   if(g.op!=2)need[g.b]=std::min(need[g.b],need[id]-d);
 }
 Builder b(old);
 std::vector<int> mapped(old.nodes.size(),-1);
 std::iota(mapped.begin(),mapped.begin()+old.n+1,0);
 auto neg=[&](int v){if(v>old.n && b.graph.nodes[v].op==2)return b.graph.nodes[v].a;return b.add(2,v,0,c);};
 for(int id=old.n+1;id<int(old.nodes.size());++id){
  auto g=old.nodes[id];int a=mapped[g.a],q=g.op==2?0:mapped[g.b];
  if(g.op==2){mapped[id]=neg(a);continue;}
  int org=b.add(g.op,a,q,c);int root=org;
  if((g.op==0 && c.aa>c.ao+3*c.an)||(g.op==1 && c.ao>c.aa+3*c.an)){
   int na=neg(a),nb=neg(q),alt=neg(b.add(1-g.op,na,nb,c));
   if(b.ready[alt]<=need[id])root=alt;
  }
  mapped[id]=root;
 }
 std::vector<int> outputs;
 for(int o:old.outputs)outputs.push_back(mapped[o]);
 return b.finish(std::move(outputs));
}
} // namespace r51
