// Factor truth tables whose cofactors take only two distinct values.
// Signal lists use little-endian truth-table indexing throughout this pass.
#pragma once

class SharedFactors {
    using Bits = std::vector<unsigned char>;
    const Case &c;
    const r51::Costs costs;
    Clock::time_point end;
    std::vector<uint32_t> blocks;
    r51::Builder gates;
    int one;

    int neg(int x) {
        const auto g = gates.graph.nodes[x];
        return x > c.n && g.op == 2 ? g.a : gates.add(2, x, 0, costs);
    }
    int binary(int op, int x, int y) {
        if (x == y) return x;
        if (!x || !y) return op ? std::max(x, y) : 0;
        if (x == one || y == one) return op ? one : (x == one ? y : x);
        return gates.add(op, x, y, costs);
    }
    int parity(int x, int y) {
        return binary(0, binary(1, x, y), neg(binary(0, x, y)));
    }
    static std::vector<int> positions(int size, const std::vector<int> &indices) {
        std::vector<int> offsets(size, 0);
        for (int i = 1; i < size; ++i)
            offsets[i] = offsets[i & (i-1)] | (1 << indices[__builtin_ctz(i)]);
        return offsets;
    }
    int small(const std::vector<int> &pins, const Bits &f) {
        if (pins.empty()) return f[0] ? one : 0;
        // Parity and majority have compact implementations for any signal names.
        bool affine = true, majority = pins.size() == 3;
        for (unsigned i = 0; i < f.size(); ++i) {
            affine &= (f[i] ^ f[0]) == __builtin_parity(i);
            majority &= (f[i] ^ f[0]) == (__builtin_popcount(i) >= 2);
        }
        if (affine || majority) {
            auto ordered = pins;
            std::stable_sort(ordered.begin(), ordered.end(), [&](int a, int b) {
                return gates.ready[a] < gates.ready[b];
            });
            int v = ordered[0];
            if (majority) {
                int a = ordered[0], b = ordered[1], last = ordered[2];
                v = binary(1, binary(0, a, b), binary(0, binary(1, a, b), last));
            } else {
                for (size_t i = 1; i < ordered.size(); ++i) v = parity(v, ordered[i]);
            }
            return f[0] ? neg(v) : v;
        }
        auto rest = pins;
        int s = rest.back();
        rest.pop_back();
        auto mid = f.begin() + f.size()/2;
        int lo = small(rest, Bits(f.begin(), mid)), hi = small(rest, Bits(mid, f.end()));
        if (lo == hi) return lo;
        return binary(1, binary(0, neg(s), lo), binary(0, s, hi));
    }
    int factor(const std::vector<int> &pins, const Bits &f) {
        if (Clock::now() >= end) throw Timeout{};
        if (pins.size() <= 3) return small(pins, f);
        uint32_t available = 0;
        for (int v : pins) if (v > 0 && v <= c.n) available |= 1u << (v-1);
        for (uint32_t mask : blocks) {
            if ((mask & available) != mask || __builtin_popcount(mask) == int(pins.size())) continue;
            std::vector<int> bi, fi, bound, free;
            for (size_t i = 0; i < pins.size(); ++i) {
                const bool in = pins[i] > 0 && pins[i] <= c.n && (mask & (1u << (pins[i]-1)));
                (in ? bi : fi).push_back(int(i));
                (in ? bound : free).push_back(pins[i]);
            }
            auto bs = positions(1 << bound.size(), bi), fs = positions(1 << free.size(), fi);
            Bits h(bs.size()), quotient;
            bool two = false, possible = true;
            Bits row(fs.size());
            for (size_t b = 0; b < bs.size() && possible; ++b) {
                for (size_t i = 0; i < fs.size(); ++i) row[i] = f[bs[b] | fs[i]];
                if (b == 0) { quotient = row; continue; }
                if (std::equal(row.begin(), row.end(), quotient.begin())) continue;
                if (!two) { quotient.insert(quotient.end(), row.begin(), row.end()); two = true; }
                else if (!std::equal(row.begin(), row.end(), quotient.begin()+fs.size())) possible = false;
                h[b] = 1;
            }
            if (!possible || !two) continue;
            int shared = factor(bound, h);
            if (shared < 0) continue;
            free.push_back(shared);
            // The selector can itself be a primary input after simplification.
            int result = factor(free, quotient);
            if (result >= 0) return result;
        }
        return -1;
    }

public:
    SharedFactors(const Case &input, const r51::Graph &old, r51::Costs prices, Clock::time_point until)
        : c(input), costs(prices), end(until), gates(old), one(neg(0)) {
        for (int i = 0; i < c.m; ++i) for (int j = 0; j < i; ++j) {
            auto common = c.table.support[i] & c.table.support[j];
            if (__builtin_popcount(common) >= 2) blocks.push_back(common);
        }
        std::sort(blocks.begin(), blocks.end(), [](auto a, auto b) {
            int x = __builtin_popcount(a), y = __builtin_popcount(b);
            return x != y ? x > y : a < b;
        });
        blocks.erase(std::unique(blocks.begin(), blocks.end()), blocks.end());
    }
    r51::Graph run(const r51::Graph &old) {
        if (blocks.empty()) return {};
        std::vector<int> outputs(c.m, -1);
        bool changed = false;
        for (int j = 0; j < c.m; ++j) {
            auto support = c.table.support[j];
            if (__builtin_popcount(support) > 10) continue;
            std::vector<int> pins, indices;
            for (int i = 0; i < c.n; ++i) if ((support >> i) & 1) {
                pins.push_back(i+1);
                indices.push_back(c.n-1-i);
            }
            Bits f;
            for (int row : positions(1 << pins.size(), indices)) f.push_back(c.table.bit(row, j));
            outputs[j] = factor(pins, f);
            changed |= outputs[j] >= 0;
        }
        if (!changed) return {};
        // Preserve incumbent cones for outputs outside the decomposition.
        std::vector<int> ids(old.nodes.size());
        std::iota(ids.begin(), ids.begin()+c.n+1, 0);
        for (size_t id = c.n+1; id < ids.size(); ++id) {
            auto g = old.nodes[id];
            ids[id] = gates.add(g.op, ids[g.a], g.op == 2 ? 0 : ids[g.b], costs);
        }
        for (int j = 0; j < c.m; ++j) if (outputs[j] < 0) outputs[j] = ids[old.outputs[j]];
        return gates.finish(std::move(outputs));
    }
};
