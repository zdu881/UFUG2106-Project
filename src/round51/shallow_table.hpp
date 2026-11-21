#pragma once

// Split the support into row selectors and column selectors. Grouping the last
// two column bits into a 4-entry LUT lets different rows share the same decoder.
static r51::Graph shallow_table(const Case &c, const r51::Costs &costs, Clock::time_point end) {
    r51::Builder b(r51::Graph(c.n, c.m));
    const int one = b.add(2, 0, 0, costs);
    auto neg = [&](int v) {
        if (!v) return one;
        if (v > c.n && b.graph.nodes[v].op == 2) return b.graph.nodes[v].a;
        return b.add(2, v, 0, costs);
    };
    auto gate = [&](int op, int a, int d) {
        const int absorb = op == 0 ? 0 : one, identity = op == 0 ? one : 0;
        if (a == absorb || d == absorb) return absorb;
        if (a == identity || a == d) return d;
        if (d == identity) return a;
        return b.add(op, a, d, costs);
    };
    auto reduce = [&](int op, const std::vector<int> &inputs) {
        using Item = std::pair<int,int>;
        std::priority_queue<Item, std::vector<Item>, std::greater<Item>> pq;
        for (int v : inputs) pq.emplace(b.ready[v], v);
        if (pq.empty()) return op == 0 ? one : 0;
        while (pq.size() > 1) {
            int a = pq.top().second; pq.pop();
            int d = pq.top().second; pq.pop();
            int v = gate(op, a, d);
            pq.emplace(b.ready[v], v);
        }
        return pq.top().second;
    };
    // Return one shared minterm per assignment, in binary input order.
    auto decode = [&](const std::vector<int> &vars, int first, int stop) {
        std::vector<std::vector<int>> parts;
        for (int i = first; i < stop; ++i) parts.push_back({neg(vars[i]+1), vars[i]+1});
        if (parts.empty()) return std::vector<int>{one};
        while (parts.size() > 1) {
            std::vector<std::vector<int>> pairs;
            for (size_t i = 0; i < parts.size(); i += 2) {
                if (i + 1 == parts.size()) { pairs.push_back(std::move(parts[i])); break; }
                std::vector<int> values;
                for (int a : parts[i]) for (int d : parts[i+1]) values.push_back(gate(0, a, d));
                pairs.push_back(std::move(values));
            }
            parts = std::move(pairs);
        }
        return std::move(parts.front());
    };
    const bool clauses = std::tie(costs.da, costs.aa) <= std::tie(costs.dor, costs.ao);
    const int reduction = clauses ? 0 : 1;
    std::vector<int> outputs;
    for (int j = 0; j < c.m; ++j) {
        std::vector<int> vars;
        for (int v = 0; v < c.n; ++v)
            if (c.table.support[j] & (uint32_t(1) << v)) vars.push_back(v);
        int k = int(vars.size());
        if (k < 2) {
            int v = k ? vars.front()+1 : 0;
            outputs.push_back(c.table.bit(0,j) ? neg(v) : v);
            continue;
        }
        int split = std::min(k/2, k-2);
        auto rows = decode(vars, 0, split);
        auto prefix = decode(vars, split, k-2);
        auto corners = decode(vars, k-2, k);
        int lut[16] = {};
        lut[15] = one;
        for (unsigned mask = 1; mask < 15; ++mask) {
            if (__builtin_popcount(mask) == 3)
                lut[mask] = neg(corners[__builtin_ctz(15U ^ mask)]);
            else if (mask == 3 || mask == 12)
                lut[mask] = mask == 3 ? neg(vars[k-2]+1) : vars[k-2]+1;
            else if (mask == 5 || mask == 10)
                lut[mask] = mask == 5 ? neg(vars[k-1]+1) : vars[k-1]+1;
            else {
                for (unsigned bit = 0; bit < 4; ++bit)
                    if ((mask >> bit) & 1) lut[mask] = gate(1, lut[mask], corners[bit]);
            }
        }
        auto offsets = [&](int first, int stop) {
            std::vector<size_t> index(size_t(1) << (stop-first));
            for (size_t i = 1; i < index.size(); ++i) {
                int v = vars[stop-1-__builtin_ctzll(i)];
                index[i] = index[i & (i-1)] | (size_t(1) << (c.n-1-v));
            }
            return index;
        };
        auto hi = offsets(0, split), lo = offsets(split, k);
        std::vector<int> outer, inner;
        for (size_t r = 0; r < rows.size(); ++r) {
            if ((r & 7) == 0 && (Clock::now() >= end || !search_memory.available())) return {};
            inner.clear();
            for (size_t g = 0; g < prefix.size(); ++g) {
                unsigned mask = 0;
                for (int t = 0; t < 4; ++t)
                    mask |= unsigned(c.table.bit(hi[r] | lo[4*g+t],j) ^ clauses) << t;
                if (!mask) continue;
                int hit = gate(0, prefix[g], lut[mask]);
                inner.push_back(clauses ? neg(hit) : hit);
            }
            int f = reduce(reduction, inner);
            outer.push_back(clauses ? gate(1, neg(rows[r]), f) : gate(0, rows[r], f));
        }
        outputs.push_back(reduce(reduction, outer));
    }
    return b.finish(std::move(outputs));
}
