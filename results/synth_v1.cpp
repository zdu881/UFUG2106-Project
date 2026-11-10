// AI-assisted experimental implementation: see docs/AI_WORK_LOG.md.
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>
using Clock=std::chrono::steady_clock;
struct Key {int op,a,b;bool operator==(const Key& k)const{return op==k.op&&a==k.a&&b==k.b;}};
struct Hash {size_t operator()(const Key& k)const{uint64_t x=uint32_t(k.op);x=x*1000003^uint32_t(k.a);x=x*1000003^uint32_t(k.b);x^=x>>30;x*=0xbf58476d1ce4e5b9ULL;x^=x>>27;return x;}};
struct Case {
 int n,m; std::vector<unsigned char> table;long long area[3],delay[3],target,limit=0,ram=0;
 explicit Case(const char* path){
  std::ifstream in(path);if(!(in>>n>>m)||n<1||n>20||m<1)throw std::runtime_error("invalid dimensions");
  size_t rows=size_t(1)<<n;table.resize(rows*size_t(m));std::string bits;
  for(size_t i=0;i<rows;++i){if(!(in>>bits)||bits.size()!=size_t(m))throw std::runtime_error("invalid table length");
   for(int j=0;j<m;++j){if(bits[j]!='0'&&bits[j]!='1')throw std::runtime_error("invalid bit");table[i*m+j]=bits[j]-'0';}}
  // op 0 NOT, 1 AND, 2 OR; official file order AND, OR, NOT.
  for(int k:{1,2,0})if(!(in>>area[k]>>delay[k])||area[k]<=0||delay[k]<=0)throw std::runtime_error("invalid gate cost");
  if(!(in>>target)||target<=0)throw std::runtime_error("invalid target");
  if(in>>limit){if(limit<0)throw std::runtime_error("invalid runtime limit");if(in>>ram)if(ram<0)throw std::runtime_error("invalid memory limit");}
 }
};
struct Timeout{};
struct BNode {int var,lo,hi;};
class BDD {
 const Case& c;const std::vector<int>& order;Clock::time_point deadline;size_t calls=0;
 std::unordered_map<Key,int,Hash> unique;
public:
 std::vector<BNode> nodes{{-1,0,0}}; // literal 0=false; 1=true; low bit complements a node
 BDD(const Case& c_,const std::vector<int>& o,Clock::time_point end):c(c_),order(o),deadline(end){}
 int build(int depth,size_t row,int output){
  if((++calls&65535)==0 && Clock::now()>deadline)throw Timeout{};
  if(depth==c.n)return c.table[row*size_t(c.m)+output];
  int lo=build(depth+1,row,output),hi=build(depth+1,row|(size_t(1)<<(c.n-1-order[depth])),output);
  if(lo==hi)return lo;
  int flip=hi&1;lo^=flip;hi^=flip; // normalized high edge is regular
  Key key{order[depth],lo,hi};auto it=unique.find(key);if(it!=unique.end())return it->second^flip;
  int lit=int(nodes.size())*2;nodes.push_back({order[depth],lo,hi});unique.emplace(key,lit);return lit^flip;
 }
};
struct Gate {int op,a,b;long long arrival;};
struct Net {
 int n;std::vector<Gate> gates;std::vector<int> outputs;long long area=0,delay=0;
 void write(const char* path)const{
  std::ofstream out(path);if(!out)throw std::runtime_error("cannot open output");
  bool one=std::find(outputs.begin(),outputs.end(),-1)!=outputs.end();
  int offset=one?1:0;
  auto id=[&](int v){return v==-1?n+1:v>n?v+offset:v;};
  if(one)out<<n+1<<" NOT 0\n";
  for(size_t j=0;j<gates.size();++j){const auto& g=gates[j];out<<n+1+int(j)+offset<<' '<<(g.op==0?"NOT":g.op==1?"AND2":"OR2")<<' '<<id(g.a);if(g.op)out<<' '<<id(g.b);out<<'\n';}
  for(size_t j=0;j<outputs.size();++j)out<<(j?" ":"")<<id(outputs[j]);out<<'\n';
  if(!out)throw std::runtime_error("output write failed");
 }
};
class Mapper {
 const Case& c;const BDD& bdd;double weight;bool pos;
 std::unordered_map<Key,int,Hash> unique;std::vector<int> cache;
 std::vector<Gate> gates;
 long long at(int v)const{return v==-1?c.delay[0]:v<=c.n?0:gates[v-c.n-1].arrival;}
 bool inverse(int a,int b)const{return (a==-1&&b==0)||(b==-1&&a==0)||(a>c.n&&gates[a-c.n-1].op==0&&gates[a-c.n-1].a==b)||(b>c.n&&gates[b-c.n-1].op==0&&gates[b-c.n-1].a==a);}
 int raw(int op,int a,int b=0){
  if(op && a>b)std::swap(a,b);
  Key k{op,a,b};auto it=unique.find(k);if(it!=unique.end())return it->second;
  long long arrival=std::max(at(a),op?at(b):0LL)+c.delay[op];int v=c.n+1+int(gates.size());
  gates.push_back({op,a,b,arrival});unique.emplace(k,v);return v;
 }
 int inv(int a){if(a==0)return -1;if(a==-1)return 0;if(a>c.n&&gates[a-c.n-1].op==0)return gates[a-c.n-1].a;return raw(0,a);}
 int basic(int op,int a,int b){
  if(a==b)return a;
  if(op==1){if(a==0||b==0)return 0;if(a==-1)return b;if(b==-1)return a;if(inverse(a,b))return 0;}
  else{if(a==-1||b==-1)return -1;if(a==0)return b;if(b==0)return a;if(inverse(a,b))return -1;}
  // Absorption: a AND (a OR b)=a, and its dual.
  for(int z=0;z<2;++z){int x=z?b:a,y=z?a:b;if(y>c.n){const auto g=gates[y-c.n-1];if(g.op==(3-op)&&(g.a==x||g.b==x))return x;}}
  return raw(op,a,b);
 }
 void rollback(size_t start){while(gates.size()>start){const auto g=gates.back();unique.erase(Key{g.op,g.a,g.b});gates.pop_back();}}
 double local_score(int root,size_t start)const{
  long long area=0;std::vector<int> stack{root};std::vector<unsigned char> seen(gates.size()-start);
  while(!stack.empty()){int v=stack.back();stack.pop_back();if(v<=c.n)continue;size_t j=size_t(v-c.n-1);if(j<start||seen[j-start])continue;seen[j-start]=1;area+=c.area[gates[j].op];stack.push_back(gates[j].a);if(gates[j].op)stack.push_back(gates[j].b);}
  return double(area)+weight*double(at(root));
 }
 // Gate polarity choice via De Morgan. Trial gates are rolled back, not retained.
 int combine(int op,int a,int b){
  size_t mark=gates.size();int direct=basic(op,a,b);double ds=local_score(direct,mark);rollback(mark);
  int na=inv(a),nb=inv(b),other=inv(basic(3-op,na,nb));double os=local_score(other,mark);rollback(mark);
  if(os+1e-9<ds){na=inv(a);nb=inv(b);return inv(basic(3-op,na,nb));}
  return basic(op,a,b);
 }
 int mux_form(int s,int lo,int hi,bool product){
  if(product){int a=combine(2,s,lo),b=combine(2,inv(s),hi);return combine(1,a,b);}
  int a=combine(1,inv(s),lo),b=combine(1,s,hi);return combine(2,a,b);
 }
public:
 Mapper(const Case& a,const BDD& b,double w,bool p):c(a),bdd(b),weight(w),pos(p),cache(b.nodes.size()*2,-2){cache[0]=0;cache[1]=-1;}
 int map(int lit){
  if(cache[lit]!=-2)return cache[lit];
  if(cache[lit^1]!=-2)return cache[lit]=inv(cache[lit^1]);
  auto node=bdd.nodes[lit/2];int low=node.lo^(lit&1),high=node.hi^(lit&1),s=node.var+1,v;
  // Map constants lazily and recognize literal functions without gates.
  if(low==0&&high==1)return cache[lit]=s;
  if(low==1&&high==0)return cache[lit]=inv(s);
  int lo=map(low),hi=map(high);
  if(low==0)v=combine(1,s,hi);
  else if(high==1)v=combine(2,s,lo);
  else if(low==1)v=combine(2,inv(s),hi);
  else if(high==0)v=combine(1,inv(s),lo);
  else{
   size_t mark=gates.size();int a=mux_form(s,lo,hi,pos);double as=local_score(a,mark);rollback(mark);
   int b=mux_form(s,lo,hi,!pos);double bs=local_score(b,mark);rollback(mark);
   v=mux_form(s,lo,hi,bs+1e-9<as?!pos:pos);
  }
  return cache[lit]=v;
 }
 Net finish(const std::vector<int>& roots){
  std::vector<int> outputs;for(int r:roots)outputs.push_back(map(r));
  std::vector<unsigned char> used(gates.size());std::vector<int> stack=outputs;
  while(!stack.empty()){int v=stack.back();stack.pop_back();if(v<=c.n)continue;size_t j=v-c.n-1;if(used[j])continue;used[j]=1;stack.push_back(gates[j].a);if(gates[j].op)stack.push_back(gates[j].b);}
  Net net;net.n=c.n;std::vector<int> ids(gates.size()+c.n+1);std::iota(ids.begin(),ids.begin()+c.n+1,0);
  for(size_t j=0;j<gates.size();++j)if(used[j]){auto g=gates[j];ids[c.n+1+j]=c.n+1+int(net.gates.size());g.a=ids[g.a];if(g.op)g.b=ids[g.b];net.area+=c.area[g.op];net.gates.push_back(g);}
  for(int v:outputs){net.outputs.push_back(v==-1?-1:ids[v]);net.delay=std::max(net.delay,at(v));}
  if(std::find(outputs.begin(),outputs.end(),-1)!=outputs.end())net.area+=c.area[0];
  return net;
 }
};
int main(int argc,char**argv){
 try{
  auto start=Clock::now();if(argc!=3)throw std::runtime_error("usage: synth INPUT OUTPUT");Case c(argv[1]);
  // Search is deterministic in order; deadline truncates it on constrained cases.
  double budget_ms=c.limit>0?std::min(1500.0,0.60*double(c.limit)):1500.0;
  auto deadline=start+std::chrono::microseconds(static_cast<long long>(budget_ms*1000));
  std::vector<int> order(c.n);std::iota(order.begin(),order.end(),0);std::mt19937 random(2106);
  Net best;bool have=false;size_t completed=0;
  auto rank=[&](const Net& x){return std::tuple<long long,long long,long long>(std::max(0LL,x.delay-c.target),x.area,x.delay);};
  for(int trial=0;trial<16;++trial){
   if(trial>0 && Clock::now()>deadline)break;
   if(trial==1)std::reverse(order.begin(),order.end());
   else if(trial>1){std::iota(order.begin(),order.end(),0);std::shuffle(order.begin(),order.end(),random);}
   try{
    // Always produce a first solution; later candidates may be interrupted safely.
    BDD bdd(c,order,have?deadline:Clock::time_point::max());std::vector<int> roots;
    for(int j=0;j<c.m;++j)roots.push_back(bdd.build(0,0,j));
    for(double w:{0.0,1.0,4.0,16.0}){
     if(have && Clock::now()>deadline)break;
     Mapper mapper(c,bdd,w,false);Net candidate=mapper.finish(roots);++completed;
     if(!have||rank(candidate)<rank(best)){best=std::move(candidate);have=true;}
    }
   }catch(const Timeout&){break;}
  }
  if(!have)throw std::runtime_error("no circuit constructed");best.write(argv[2]);
  std::cerr<<"candidates="<<completed<<" area="<<best.area<<" delay="<<best.delay<<" target="<<c.target<<'\n';
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
 return 0;
}
