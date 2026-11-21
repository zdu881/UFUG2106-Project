#pragma once
#include "typed_netgraph.hpp"
#include <array>

namespace round4_phase {
struct Ref {int node, flip;};
struct Gate {int op; Ref a,b;};
class Engine {
 public:
 int n,m;
 r51::Costs costs;
 std::vector<Gate> graph;
 std::vector<Ref> outputs;
 std::vector<int> phase;
 Engine(const r51::Graph &net, const r51::Costs &c)
     : n(net.n), m(net.m), costs(c), graph(n+1) {
     std::vector<Ref> alias(net.nodes.size());
     for(int id=0;id<=n;++id)alias[id]={id,0};
     for(int id=n+1;id<int(net.nodes.size());++id) {
         const auto &g=net.nodes[id];
         if(g.op==2) {
             alias[id]=alias[g.a];
             alias[id].flip^=1;
         } else {
             alias[id]={int(graph.size()),0};
             graph.push_back({g.op,alias[g.a],alias[g.b]});
         }
     }
     for(int id:net.outputs)outputs.push_back(alias[id]);
     phase.resize(graph.size());
 }

 void improve(int passes) {
     // A complemented copy costs one NOT if any live consumer needs it.
     std::vector<std::array<int,2>> demand(graph.size());
     for(const auto &out:outputs)++demand[out.node][out.flip];
     for(int v=n+1;v<int(graph.size());++v)
         for(Ref r:{graph[v].a,graph[v].b})++demand[r.node][phase[v]^r.flip];

     auto switch_inputs = [&](const Gate &g,int previous) {
         int extra_inverters=0;
         for(Ref r:{g.a,g.b}) {
             auto &count=demand[r.node];
             const int opposite=phase[r.node]^1;
             extra_inverters-=count[opposite]>0;
             --count[previous^r.flip];
             ++count[previous^r.flip^1];
             extra_inverters+=count[opposite]>0;
         }
         return extra_inverters;
     };
     const int prices[]={costs.aa,costs.ao};
     for(int pass=0;pass<passes;++pass) {
         bool changed=false;
         for(int i=n+1;i<int(graph.size());++i) {
             int v=pass%2 ? int(graph.size())-1-(i-n-1) : i;
             const auto &g=graph[v];
             int old=phase[v], next=old^1;
             int inversions=(demand[v][old]>0)-(demand[v][next]>0);
             inversions+=switch_inputs(g,old);
             long long delta=prices[g.op^next]-prices[g.op^old]+1LL*costs.an*inversions;
             if(delta<0) {
                 phase[v]=next;
                 changed=true;
             } else {
                 switch_inputs(g,next);
             }
         }
         if(!changed)break;
     }
 }
 r51::Graph construct()const{
   r51::Builder builder(r51::Graph(n,m));
   builder.intern.reserve(graph.size()*2);
   const auto &gates=builder.graph.nodes;
   auto raw=[&](int op,int a,int b){return builder.add(op,a,b,costs);};
   std::vector<int> mapped(graph.size());
   std::iota(mapped.begin(),mapped.begin()+n+1,0);
   int one=-1;
   auto ensure_one=[&](){if(one<0)one=raw(2,0,0);return one;};
   auto neg=[&](int a){
     if(a==0)return ensure_one();
     if(one>=0&&a==one)return 0;
     if(a>n&&gates[a].op==2)return gates[a].a;
     return raw(2,a,0);
   };
   auto opposite=[&](int a,int b){
     if(a==0&&one>=0&&b==one)return true;
     if(b==0&&one>=0&&a==one)return true;
     return (a>n&&gates[a].op==2&&gates[a].a==b)||
            (b>n&&gates[b].op==2&&gates[b].a==a);
   };
   auto combine=[&](int op,int a,int b){
     if(op==0){
       if(a==0||b==0)return 0;
       if(one>=0&&a==one)return b;
       if(one>=0&&b==one)return a;
       if(a==b)return a;
       if(opposite(a,b))return 0;
     }else{
       if(one>=0&&(a==one||b==one))return one;
       if(a==0)return b;
       if(b==0)return a;
       if(a==b)return a;
       if(opposite(a,b))return ensure_one();
     }
     return raw(op,a,b);
   };
   for(int v=n+1;v<int(graph.size());++v){
     const auto &g=graph[v];int pv=phase[v];
     int wantA=pv^g.a.flip, wantB=pv^g.b.flip;
     int a=mapped[g.a.node],b=mapped[g.b.node];
     if(wantA!=phase[g.a.node])a=neg(a);
     if(wantB!=phase[g.b.node])b=neg(b);
     mapped[v]=combine(g.op^pv,a,b);
   }
   std::vector<int> out;
   for(const auto &r:outputs){int v=mapped[r.node];if(r.flip!=phase[r.node])v=neg(v);out.push_back(v);}
   return builder.finish(std::move(out));
 }
};
} // namespace round4_phase
