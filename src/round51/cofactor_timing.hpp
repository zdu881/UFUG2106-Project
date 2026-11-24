// Timing rescue for small output supports. A two-class cofactor partition
// proves F(bound,free) = G(selector(bound),free) on every assignment.
#pragma once
#include <map>

class CofactorTiming {
    using Bits = std::vector<unsigned char>;
    struct Signal { int pos, neg; };
    using Pins = std::vector<Signal>;
    r51::Builder &b;
    const r51::Costs &cost;
    Clock::time_point end;
    int one;

    int invert(int x) {
        const auto g = b.graph.nodes[x];
        return g.op == 2 ? g.a : b.add(2,x,0,cost);
    }
    int gate(int op, int x, int y) {
        if (x == y) return x;
        if (x == 0 || y == 0) return op ? std::max(x,y) : 0;
        if (x == one || y == one) return op ? one : (x == one ? y : x);
        return b.add(op,x,y,cost);
    }
    int faster(int x, int y) const {
        return std::tie(b.ready[x],x) <= std::tie(b.ready[y],y) ? x : y;
    }
    static Signal flip(Signal x) { return {x.neg,x.pos}; }
    Signal choose(Signal a, Signal z) const {
        return {faster(a.pos,z.pos),faster(a.neg,z.neg)};
    }
    Signal phases(int p, int n) {
        return {faster(p,invert(n)),faster(n,invert(p))};
    }
    Signal land(Signal a, Signal z) {
        return phases(gate(0,a.pos,z.pos),gate(1,a.neg,z.neg));
    }
    Signal lor(Signal a, Signal z) { return flip(land(flip(a),flip(z))); }
    Signal parity(Signal a, Signal z) {
        auto p = lor(land(a,flip(z)),land(flip(a),z));
        auto n = lor(land(a,z),land(flip(a),flip(z)));
        return choose(p,flip(n));
    }
    static Bits split(const Bits &f, int v, int bit) {
        int step = 1<<v;
        Bits out;
        out.reserve(f.size()/2);
        for (size_t i=bit*step; i<f.size(); i+=2*step)
            out.insert(out.end(),f.begin()+i,f.begin()+i+step);
        return out;
    }
    static int support(const Bits &f) {
        int mask = 0;
        for (int step=1; step<int(f.size()); step*=2)
            for (int j=0; j<int(f.size()); j+=2*step)
                if (!std::equal(f.begin()+j,f.begin()+j+step,f.begin()+j+step)) {
                    mask |= step;
                    break;
                }
        return mask;
    }
    static std::vector<int> offsets(int mask) {
        std::vector<int> result{0};
        for (int left=mask; left; left&=left-1) {
            int step = left & -left, count=int(result.size());
            for (int j=0; j<count; ++j) result.push_back(result[j]+step);
        }
        return result;
    }
    // Mixed 2-by-2 slices rule out partitions; the full reconstruction check
    // below handles zeros, shared variables, and degenerate slices exactly.
    bool bisplit(const Pins &pins,const Bits &f,int width,Signal &result) {
        const int n=int(pins.size()), full=(1<<n)-1;
        std::array<std::vector<int>,3> edges;
        for (auto &g:edges) g.resize(n);
        for (int i=0;i<n;++i) for (int j=0;j<i;++j) {
            int a=1<<i, z=1<<j, bad=0;
            for (int k=0;k<=full && bad!=7;++k) if (!(k&(a|z))) {
                int t=f[k], u=f[k|a], v=f[k|z], w=f[k|a|z];
                if ((t&w)!=(u&v)) bad|=1;
                if ((t|w)!=(u|v)) bad|=2;
                if (t^u^v^w) bad|=4;
            }
            for (int op=0;op<3;++op) if (bad&(1<<op)) {
                edges[op][i]|=z; edges[op][j]|=a;
            }
        }
        using Split=std::tuple<int,int,int,int,int>;
        std::vector<Split> choices;
        for (int shared=0;shared<full;++shared) {
            int common=__builtin_popcount(unsigned(shared));
            if (common>3 || n-common<2) continue;
            for (int op=0;op<3;++op) {
                int left=full^shared;
                std::vector<int> parts;
                while (left) {
                    int group=0, fringe=left & -left;
                    while (fringe) {
                        int bit=fringe & -fringe;
                        group|=bit;
                        fringe=(fringe| (edges[op][__builtin_ctz(bit)] & left)) & ~group;
                    }
                    parts.push_back(group); left &= ~group;
                }
                if (parts.size()<2) continue;
                // Balance independent groups before combining them into a tree.
                std::sort(parts.begin(),parts.end(),[](int a,int b){
                    return __builtin_popcount(unsigned(a))>__builtin_popcount(unsigned(b));
                });
                int a=0,b=0;
                for (int part:parts)
                    (__builtin_popcount(unsigned(a))<=__builtin_popcount(unsigned(b)) ? a:b)|=part;
                int size=std::max(__builtin_popcount(unsigned(a)),__builtin_popcount(unsigned(b)));
                choices.emplace_back(common, size, op, a|shared, b|shared);
            }
        }
        std::sort(choices.begin(),choices.end());
        for (auto [common,size,op,am,bm]:choices) {
            if (Clock::now()>=end) throw Timeout{};
            auto ao=offsets(am),bo=offsets(bm);
            Bits af(ao.size(),op==1),bf(bo.size(),op==1);
            auto pack=[&](int mask) {
                std::vector<int> ids(f.size());
                for (int k=1;k<=full;++k) {
                    int bit=k & -k;
                    ids[k]=ids[k^bit]+((mask & bit) ? 1<<__builtin_popcount(unsigned(mask & (bit-1))) : 0);
                }
                return ids;
            };
            auto ai=pack(am),bi=pack(bm);
            if (op==2) {
                for (size_t i=0;i<ao.size();++i) af[i]=f[ao[i]];
                for (size_t i=0;i<bo.size();++i) bf[i]=f[bo[i]]^f[bo[i]&am];
            } else {
                for (int k=0;k<=full;++k) {
                    if (op==0) { af[ai[k]]|=f[k]; bf[bi[k]]|=f[k]; }
                    else { af[ai[k]]&=f[k]; bf[bi[k]]&=f[k]; }
                }
            }
            bool exact=true;
            for (int k=0;k<=full;++k) {
                int a=af[ai[k]], b=bf[bi[k]];
                if ((op==0 ? a&b : op==1 ? a|b : a^b)!=f[k]) {exact=false;break;}
            }
            if (!exact) continue;
            Pins ap,bp;
            for (int i=0;i<n;++i) {
                if (am & (1<<i)) ap.push_back(pins[i]);
                if (bm & (1<<i)) bp.push_back(pins[i]);
            }
            auto a=solve(ap,std::move(af),width), z=solve(bp,std::move(bf),width);
            result=op==0 ? land(a,z) : op==1 ? lor(a,z) : parity(a,z);
            return true;
        }
        return false;
    }
    Signal solve(Pins pins, Bits f, int width) {
        if (Clock::now() >= end || b.graph.nodes.size() >= 45000) throw Timeout{};
        int used = support(f);
        for (int v=int(pins.size())-1; v>=0; --v) if (!(used & (1<<v))) {
            f = split(f,v,0);
            pins.erase(pins.begin()+v);
        }
        const int n=int(pins.size());
        if (!n) return f[0] ? Signal{one,0} : Signal{0,one};
        if (n==1) return f[0] ? flip(pins[0]) : pins[0];
        if (n==2) {
            int ones=std::accumulate(f.begin(),f.end(),0);
            if (ones==2) {
                auto p=parity(pins[0],pins[1]);
                return f[0] ? flip(p) : p;
            }
            int row=int(std::find(f.begin(),f.end(),ones==1)-f.begin());
            auto p=land(row&1 ? pins[0] : flip(pins[0]),row&2 ? pins[1] : flip(pins[1]));
            return ones==1 ? p : flip(p);
        }
        Signal factored{};
        if (bisplit(pins,f,width,factored)) return factored;
        int full=(1<<n)-1;
        // Try pairs first to expose common primitive gates. Also try a
        // partition with just one free variable, e.g. F = A OR h(rest).
        for (int size : {2,n-1,3}) {
            if (size>=n) continue;
            for (int mask=1; mask<full; ++mask) {
                if (__builtin_popcount(unsigned(mask))!=size) continue;
                if (Clock::now()>=end) throw Timeout{};
                auto bound=offsets(mask), free=offsets(full^mask);
                Bits h(bound.size()), quotient;
                int second=-1;
                bool possible=true;
                for (int i=1; i<int(bound.size()) && possible; ++i) {
                    bool first=true,other=true;
                    for (int z:free) {
                        auto y=f[bound[i]|z];
                        first &= y==f[z];
                        if (second>=0) other &= y==f[bound[second]|z];
                        if (!first && (second<0 || !other)) break;
                    }
                    if (!first) {
                        if (second<0) second=i;
                        else if (!other) possible=false;
                        h[i]=1;
                    }
                }
                if (!possible || second<0) continue;
                for (int z:free) quotient.push_back(f[z]);
                for (int z:free) quotient.push_back(f[bound[second]|z]);
                Pins bp,fp;
                for (int i=0;i<n;++i) ((mask>>i)&1 ? bp:fp).push_back(pins[i]);
                fp.push_back(solve(bp,std::move(h),width));
                return solve(fp,std::move(quotient),width);
            }
        }
        // For an irreducible function choose Shannon variables by the larger
        // of the two remaining supports. Try a second order on small cones.
        std::vector<std::tuple<int,int,int>> orders;
        for (int v=0;v<n;++v) {
            int l=__builtin_popcount(unsigned(support(split(f,v,0))));
            int h=__builtin_popcount(unsigned(support(split(f,v,1))));
            orders.emplace_back(std::max(l,h),l+h,v);
        }
        std::sort(orders.begin(),orders.end());
        Signal best{};
        for (int j=0;j<(width && n<=8 ? 2:1);++j) {
            int v=std::get<2>(orders[j]);
            auto x=pins[v]; auto rest=pins; rest.erase(rest.begin()+v);
            auto low=split(f,v,0),high=split(f,v,1);
            bool up=true,down=true;
            for (size_t i=0;i<low.size();++i) { up &= low[i]<=high[i]; down &= high[i]<=low[i]; }
            auto a=solve(rest,std::move(low),std::max(0,width-1));
            auto z=solve(rest,std::move(high),std::max(0,width-1));
            auto y=lor(up ? a : land(flip(x),a), down ? z : land(x,z));
            auto dual=land(down ? a : lor(x,a), up ? z : lor(flip(x),z));
            y=choose(y,dual);
            best=j ? choose(best,y) : y;
        }
        return best;
    }
    Signal tree(std::vector<Signal> terms,bool conjunction) {
        if (terms.empty()) return conjunction ? Signal{one,0}:Signal{0,one};
        using Item=std::tuple<int,int,int>;
        std::priority_queue<Item,std::vector<Item>,std::greater<Item>> q;
        auto push=[&](Signal s){q.emplace(std::min(b.ready[s.pos],b.ready[s.neg]),s.pos,s.neg);};
        auto pop=[&](){auto [t,p,n]=q.top();q.pop();return Signal{p,n};};
        for (auto t:terms) push(t);
        while (q.size()>1) { auto a=pop(),z=pop(); push(conjunction ? land(a,z):lor(a,z)); }
        return pop();
    }
    Signal product(const Pins &pins,int fixed,int value) {
        Pins terms;
        for (size_t i=0;i<pins.size();++i) if ((fixed>>i)&1)
            terms.push_back((value>>i)&1 ? pins[i]:flip(pins[i]));
        return tree(std::move(terms),true);
    }
    // Expand actual minterms into implicants; every expansion checks the
    // full subcube. Cover both phases and keep the lower arrival for each.
    Signal cover(const Pins &pins,const Bits &f) {
        if (Clock::now()>=end || b.graph.nodes.size()>=60000) throw Timeout{};
        int n=int(pins.size()), full=int(f.size())-1;
        std::array<Signal,2> phase;
        for (int on=0;on<2;++on) {
            Bits used(f.size()); Pins terms;
            for (int row=0;row<=full;++row) {
                if (!(row % 64) && (Clock::now() >= end || b.graph.nodes.size() >= 60000))
                    throw Timeout{};
                if (f[row]!=on || used[row]) continue;
                int free=0;
                for (int v=0;v<n;++v) {
                    int bit=1<<v;
                    bool ok=true;
                    for (int j=free;;j=(j-1)&free) {
                        if (f[row^j^bit]!=on) {ok=false;break;}
                        if (!j) break;
                    }
                    if (ok) free|=bit;
                }
                for (int j=free;;j=(j-1)&free) {used[row^j]=1;if (!j) break;}
                terms.push_back(product(pins,full^free,row));
            }
            phase[on]=tree(std::move(terms),false);
        }
        return choose(phase[1],flip(phase[0]));
    }
    // Parallel Shannon expansion: each distinct residual function needs one
    // gate with its selector. Equal cofactors share the same residual circuit.
    Signal block_map(const Pins &pins,const Bits &f,int mask) {
        int full=int(f.size())-1;
        auto rows=offsets(mask), cols=offsets(full^mask);
        Pins a,bp;
        for (size_t i=0;i<pins.size();++i) ((mask>>i)&1 ? a:bp).push_back(pins[i]);
        std::map<Bits,std::vector<int>> groups;
        for (size_t i=0;i<rows.size();++i) {
            Bits col;
            for (int z:cols) col.push_back(f[rows[i]|z]);
            groups[std::move(col)].push_back(int(i));
        }
        Pins positives,negatives;
        for (const auto &[col,which]:groups) {
            Signal selector;
            if (which.size()==1) selector=product(a,int(rows.size())-1,which[0]);
            else {
                Bits select(rows.size());
                for (int i:which) select[i]=1;
                selector=cover(a,select);
            }
            auto val=cover(bp,col);
            positives.push_back(land(selector,val));
            negatives.push_back(land(selector,flip(val)));
        }
        return choose(tree(std::move(positives),false),flip(tree(std::move(negatives),false)));
    }
public:
    CofactorTiming(r51::Builder &builder,const r51::Costs &prices,Clock::time_point until)
        : b(builder), cost(prices), end(until), one(invert(0)) {}
    int output(const Case &c,int j,int target) {
        Pins pins;
        int mask=0;
        for (int bit=0;bit<c.n;++bit) {
            int id=c.n-bit;
            if (c.table.support[j] & (1u<<(id-1))) {
                pins.push_back({id,invert(id)});
                mask|=1<<bit;
            }
        }
        Bits f;
        for (int row:offsets(mask)) f.push_back(c.table.bit(row,j));
        auto best=solve(pins,f,2);
        try {
            best=choose(best,cover(pins,f));
            if (pins.size()>3) {
                unsigned full=(1u<<pins.size())-1;
                std::vector<std::pair<int,int>> influence;
                for (size_t i=0;i<pins.size();++i) {
                    int changes=0;
                    for (unsigned x=0;x<=full;++x) changes+=f[x]!=f[x^(1u<<i)];
                    influence.emplace_back(changes,i);
                }
                std::sort(influence.begin(),influence.end());
                unsigned top=0;
                for (size_t i=0;i<pins.size()/2;++i) top|=1u<<influence[i].second;
                unsigned half=(1u<<(pins.size()/2))-1;
                for (unsigned split_mask:{top,full^top,half,full^half})
                    best=choose(best,block_map(pins,f,split_mask));
                // Small supports admit more balanced partitions within the same time budget.
                if (pins.size() <= 10) {
                    for (unsigned mask = 1; mask < full && b.ready[best.pos] > target; ++mask) {
                        if (std::abs(2 * __builtin_popcount(mask) - int(pins.size())) > 2) continue;
                        best = choose(best, block_map(pins, f, mask));
                    }
                }
            }
        } catch (const Timeout &) {}
        return best.pos;
    }
};

static r51::Graph rescue_late_outputs(const Case &c,const Net &best,const r51::Costs &costs,
                                      int target,Clock::time_point end) {
    auto old=typed_net(best);
    r51::Builder b(old);
    auto import_roots = [&](const r51::Graph &graph) {
        std::vector<int> ids(graph.n + 1);
        std::iota(ids.begin(), ids.end(), 0);
        for (size_t i = graph.n + 1; i < graph.nodes.size(); ++i) {
            const auto g = graph.nodes[i];
            ids.push_back(b.add(g.op, ids[g.a], g.op == 2 ? 0 : ids[g.b], costs));
        }
        std::vector<int> roots;
        for (int output : graph.outputs) roots.push_back(ids[output]);
        return roots;
    };
    auto roots = import_roots(old);
    std::vector<int> late;
    for (int j=0;j<c.m;++j)
        if (b.ready[roots[j]]>target && __builtin_popcount(c.table.support[j])<=14) late.push_back(j);
    bool changed=false;
    for (size_t i=0;i<late.size();++i) {
        auto now=Clock::now();
        if (now>=end) break;
        // Drop discarded candidates before searching the next output.
        if (b.graph.nodes.size() > 20000) {
            auto live = b.finish(roots);
            b = r51::Builder(live);
            roots = import_roots(live);
        }
        int j=late[i];
        CofactorTiming search(b,costs,now+(end-now)/(late.size()-i));
        try {
            int alt=search.output(c,j,target);
            if (b.ready[alt]<b.ready[roots[j]]) { roots[j]=alt; changed=true; }
        } catch (const Timeout &) {}
    }
    return changed ? b.finish(std::move(roots)) : r51::Graph{};
}
