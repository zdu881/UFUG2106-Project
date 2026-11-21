#pragma once
#include "typed_netgraph.hpp"
#include <functional>
namespace round4_algebra {
inline r51::Graph rewrite(const r51::Graph &net, const r51::Costs &costs) {
 const int n=net.n;
 const auto &old=net.nodes;
 r51::Builder builder(net);
 const auto &g=builder.graph.nodes;
 auto raw=[&](int op,int a,int b){return builder.add(op,a,b,costs);};
 std::vector<int> map(old.size());
 std::iota(map.begin(),map.begin()+n+1,0);
 int ONE=-1;
 std::function<int(int)> neg=[&](int a)->int {
  if(a==0){if(ONE<0)ONE=raw(2,0,0);return ONE;}
  if(ONE>=0&&a==ONE)return 0;
  if(a>n && g[a].op==2)return g[a].a;
  return raw(2,a,0);
 };
 auto inverse=[&](int a,int b)->bool{
  if(a==0&&ONE>=0&&b==ONE)return true;
  if(b==0&&ONE>=0&&a==ONE)return true;
  return (a>n&&g[a].op==2&&g[a].a==b)||(b>n&&g[b].op==2&&g[b].a==a);
 };
 std::function<int(int,int,int)> combine;
 combine=[&](int op,int a,int b)->int {
  if(op==0){
   if(a==0||b==0)return 0;
   if(ONE>=0&&a==ONE)return b;
   if(ONE>=0&&b==ONE)return a;
  }else{
   if(ONE>=0&&(a==ONE||b==ONE))return ONE;
   if(a==0)return b;
   if(b==0)return a;
  }
  if(a==b)return a;
  if(inverse(a,b))return op==0?0:neg(0);
  auto contains=[&](int v,int gop,int needle)->bool{
    return v>n && g[v].op==gop && (g[v].a==needle || g[v].b==needle);
  };
  // Absorption: a&(a|b)=a or a|(a&b)=a.
  if(contains(a,1-op,b))return b;
  if(contains(b,1-op,a))return a;
  // Complement absorption: a&(not(a)|b)=a&b.
  {
   auto half_absorb=[&](int outer,int inner)->int{
     if(inner<=n||g[inner].op!=1-op)return -1;
     auto p=g[inner].a,q=g[inner].b;
     if(inverse(outer,p))return combine(op,outer,q);
     if(inverse(outer,q))return combine(op,outer,p);
     return -1;
   };
   int u=half_absorb(a,b);if(u>=0)return u;
   u=half_absorb(b,a);if(u>=0)return u;
  }
  // Boolean distributivity: (a&b)|(a&c)=a&(b|c), and dual.
  if(a>n && b>n && g[a].op==1-op && g[b].op==1-op){
    int cands[2]={g[a].a,g[a].b};
    for(int common:cands){
      int otherA=(g[a].a==common?g[a].b:g[a].a);
      int otherB=-1;
      if(g[b].a==common)otherB=g[b].b;
      if(g[b].b==common)otherB=g[b].a;
      if(otherB<0)continue;
      return combine(1-op,common,combine(op,otherA,otherB));
    }
  }
  return raw(op,a,b);
 };
 for(int id=n+1;id<int(old.size());++id){
   auto u=old[id];map[id]=(u.op==2?neg(map[u.a]):combine(u.op,map[u.a],map[u.b]));
 }
 auto outputs=net.outputs;
 for(int &o:outputs)o=map[o];
 return builder.finish(std::move(outputs));
}
}
