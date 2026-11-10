// AI-assisted experimental implementation: see docs/AI_WORK_LOG.md.
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <numeric>
#include <queue>
#include <random>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>
using Clock = std::chrono::steady_clock;
struct Key {
    int op, a, b;
    bool operator==(const Key &k) const {
        return op == k.op && a == k.a && b == k.b;
    }
};
struct Hash {
    size_t operator()(const Key &k) const {
        uint64_t x = uint32_t(k.op);
        x = x * 1000003 ^ uint32_t(k.a);
        x = x * 1000003 ^ uint32_t(k.b);
        x ^= x >> 30;
        x *= 0xbf58476d1ce4e5b9ULL;
        x ^= x >> 27;
        return x;
    }
};
struct Case {
    int n, m;
    std::vector<unsigned char> table;
    long long area[3], delay[3], target, limit = 0, ram = 0;
    explicit Case(const char *path) {
        std::ifstream in(path);
        if (!(in >> n >> m) || n < 1 || n > 20 || m < 1)
            throw std::runtime_error("invalid dimensions");
        size_t rows = size_t(1) << n;
        table.resize(rows * size_t(m));
        std::string bits;
        for (size_t i = 0; i < rows; ++i) {
            if (!(in >> bits) || bits.size() != size_t(m))
                throw std::runtime_error("invalid table length");
            for (int j = 0; j < m; ++j) {
                if (bits[j] != '0' && bits[j] != '1')
                    throw std::runtime_error("invalid bit");
                table[i * m + j] = bits[j] - '0';
            }
        }
        // op 0 NOT, 1 AND, 2 OR; official file order AND, OR, NOT.
        for (int k : {1, 2, 0})
            if (!(in >> area[k] >> delay[k]) || area[k] <= 0 || delay[k] <= 0)
                throw std::runtime_error("invalid gate cost");
        if (!(in >> target) || target <= 0)
            throw std::runtime_error("invalid target");
        if (in >> limit) {
            if (limit < 0)
                throw std::runtime_error("invalid runtime limit");
            if (in >> ram)
                if (ram < 0)
                    throw std::runtime_error("invalid memory limit");
        }
    }
};
struct Timeout {};
struct BNode {
    int var, lo, hi;
};
class BDD {
    const Case &c;
    const std::vector<int> &order;
    Clock::time_point deadline;
    size_t calls = 0;
    std::unordered_map<Key, int, Hash> unique;
    std::unordered_map<Key, bool, Hash> implications;
    std::vector<int> level;
    size_t implication_calls = 0;

  public:
    std::vector<BNode> nodes{{-1, 0, 0}}; // literal 0=false; 1=true; low bit complements a node
    BDD(const Case &c_, const std::vector<int> &o, Clock::time_point end)
        : c(c_), order(o), deadline(end), level(c_.n) {
        for (int j = 0; j < c.n; ++j)
            level[o[j]] = j;
    }
    bool implies(int a, int b) {
        if (a == 0 || b == 1 || a == b)
            return true;
        if (a == 1 || b == 0 || a == (b ^ 1))
            return false;
        if (++implication_calls > 200000)
            return false; // conservative optimization cutoff
        Key key{-1, a, b};
        auto it = implications.find(key);
        if (it != implications.end())
            return it->second;
        auto x = nodes[a / 2], y = nodes[b / 2];
        int top = std::min(level[x.var], level[y.var]);
        int a0 = a, a1 = a, b0 = b, b1 = b;
        if (level[x.var] == top) {
            a0 = x.lo ^ (a & 1);
            a1 = x.hi ^ (a & 1);
        }
        if (level[y.var] == top) {
            b0 = y.lo ^ (b & 1);
            b1 = y.hi ^ (b & 1);
        }
        bool result = implies(a0, b0) && implies(a1, b1);
        implications.emplace(key, result);
        return result;
    }
    int build(int depth, size_t row, int output) {
        if ((++calls & 65535) == 0 && Clock::now() > deadline)
            throw Timeout{};
        if (depth == c.n)
            return c.table[row * size_t(c.m) + output];
        int lo = build(depth + 1, row, output),
            hi = build(depth + 1, row | (size_t(1) << (c.n - 1 - order[depth])), output);
        if (lo == hi)
            return lo;
        int flip = hi & 1;
        lo ^= flip;
        hi ^= flip; // normalized high edge is regular
        Key key{order[depth], lo, hi};
        auto it = unique.find(key);
        if (it != unique.end())
            return it->second ^ flip;
        int lit = int(nodes.size()) * 2;
        nodes.push_back({order[depth], lo, hi});
        unique.emplace(key, lit);
        return lit ^ flip;
    }
};
struct Gate {
    int op, a, b;
    long long arrival;
};
struct Net {
    int n;
    std::vector<Gate> gates;
    std::vector<int> outputs;
    long long area = 0, delay = 0;
    void write(const char *path) const {
        std::ofstream out(path);
        if (!out)
            throw std::runtime_error("cannot open output");
        bool one = std::find(outputs.begin(), outputs.end(), -1) != outputs.end();
        int offset = one ? 1 : 0;
        auto id = [&](int v) { return v == -1 ? n + 1 : v > n ? v + offset : v; };
        if (one)
            out << n + 1 << " NOT 0\n";
        for (size_t j = 0; j < gates.size(); ++j) {
            const auto &g = gates[j];
            out << n + 1 + int(j) + offset << ' '
                << (g.op == 0   ? "NOT"
                    : g.op == 1 ? "AND2"
                                : "OR2")
                << ' ' << id(g.a);
            if (g.op)
                out << ' ' << id(g.b);
            out << '\n';
        }
        for (size_t j = 0; j < outputs.size(); ++j)
            out << (j ? " " : "") << id(outputs[j]);
        out << '\n';
        if (!out)
            throw std::runtime_error("output write failed");
    }
};
class Mapper {
    const Case &c;
    BDD &bdd;
    double weight;
    bool pos;
    bool plain;
    std::unordered_map<Key, int, Hash> unique;
    std::vector<int> cache;
    std::vector<uint32_t> affine;
    Clock::time_point deadline;
    size_t calls = 0;
    std::vector<Gate> gates;
    long long at(int v) const {
        return v == -1 ? c.delay[0] : v <= c.n ? 0 : gates[v - c.n - 1].arrival;
    }
    bool inverse(int a, int b) const {
        return (a == -1 && b == 0) || (b == -1 && a == 0) ||
               (a > c.n && gates[a - c.n - 1].op == 0 && gates[a - c.n - 1].a == b) ||
               (b > c.n && gates[b - c.n - 1].op == 0 && gates[b - c.n - 1].a == a);
    }
    int raw(int op, int a, int b = 0) {
        if ((++calls & 1023) == 0 && Clock::now() > deadline)
            throw Timeout{};
        if (op && a > b)
            std::swap(a, b);
        Key k{op, a, b};
        auto it = unique.find(k);
        if (it != unique.end())
            return it->second;
        long long arrival = std::max(at(a), op ? at(b) : 0LL) + c.delay[op];
        int v = c.n + 1 + int(gates.size());
        gates.push_back({op, a, b, arrival});
        unique.emplace(k, v);
        return v;
    }
    int inv(int a) {
        if (a == 0)
            return -1;
        if (a == -1)
            return 0;
        if (a > c.n && gates[a - c.n - 1].op == 0)
            return gates[a - c.n - 1].a;
        return raw(0, a);
    }
    int basic(int op, int a, int b) {
        if (a == b)
            return a;
        if (op == 1) {
            if (a == 0 || b == 0)
                return 0;
            if (a == -1)
                return b;
            if (b == -1)
                return a;
            if (inverse(a, b))
                return 0;
        } else {
            if (a == -1 || b == -1)
                return -1;
            if (a == 0)
                return b;
            if (b == 0)
                return a;
            if (inverse(a, b))
                return -1;
        }
        // Absorption: a AND (a OR b)=a, and its dual.
        for (int z = 0; z < 2; ++z) {
            int x = z ? b : a, y = z ? a : b;
            if (y > c.n) {
                const auto g = gates[y - c.n - 1];
                if (g.op == (3 - op) && (g.a == x || g.b == x))
                    return x;
            }
        }
        return raw(op, a, b);
    }
    void rollback(size_t start) {
        while (gates.size() > start) {
            const auto g = gates.back();
            unique.erase(Key{g.op, g.a, g.b});
            gates.pop_back();
        }
    }
    double local_score(int root, size_t start) const {
        long long area = 0;
        std::vector<int> stack{root};
        std::vector<unsigned char> seen(gates.size() - start);
        while (!stack.empty()) {
            int v = stack.back();
            stack.pop_back();
            if (v <= c.n)
                continue;
            size_t j = size_t(v - c.n - 1);
            if (j < start || seen[j - start])
                continue;
            seen[j - start] = 1;
            area += c.area[gates[j].op];
            stack.push_back(gates[j].a);
            if (gates[j].op)
                stack.push_back(gates[j].b);
        }
        return double(area) + weight * double(at(root));
    }
    // Gate polarity choice via De Morgan. Trial gates are rolled back, not retained.
    int combine(int op, int a, int b) {
        if (plain)
            return basic(op, a, b);
        size_t mark = gates.size();
        int direct = basic(op, a, b);
        double ds = local_score(direct, mark);
        rollback(mark);
        int na = inv(a), nb = inv(b), other = inv(basic(3 - op, na, nb));
        double os = local_score(other, mark);
        rollback(mark);
        if (os + 1e-9 < ds) {
            na = inv(a);
            nb = inv(b);
            return inv(basic(3 - op, na, nb));
        }
        return basic(op, a, b);
    }
    int mux_form(int s, int lo, int hi, bool product) {
        if (product) {
            int a = combine(2, s, lo), b = combine(2, inv(s), hi);
            return combine(1, a, b);
        }
        int a = combine(1, inv(s), lo), b = combine(1, s, hi);
        return combine(2, a, b);
    }
    int xor_gate(int a, int b) {
        if (a == b)
            return 0;
        if (inverse(a, b))
            return -1;
        auto form = [&](bool inverted) {
            if (!inverted) {
                int u = combine(2, a, b), v = inv(combine(1, a, b));
                return combine(1, u, v);
            }
            int u = combine(1, a, b), v = inv(combine(2, a, b));
            return inv(combine(2, u, v));
        };
        size_t mark = gates.size();
        int v = form(false);
        double first = local_score(v, mark);
        rollback(mark);
        v = form(true);
        double second = local_score(v, mark);
        rollback(mark);
        return form(second < first);
    }
    int map_affine(uint32_t mask) {
        using Item = std::pair<long long, int>;
        std::priority_queue<Item, std::vector<Item>, std::greater<Item>> q;
        for (int j = 0; j < c.n; ++j)
            if (mask & (1u << j))
                q.emplace(0, j + 1);
        while (q.size() > 1) {
            int a = q.top().second;
            q.pop();
            int b = q.top().second;
            q.pop();
            int v = xor_gate(a, b);
            q.emplace(at(v), v);
        }
        int v = q.empty() ? 0 : q.top().second;
        return (mask & (1u << 20)) ? inv(v) : v;
    }

  public:
    Mapper(const Case &a, BDD &b, double w, bool p, bool plain_ = false,
           Clock::time_point end = Clock::time_point::max())
        : c(a), bdd(b), weight(w), pos(p), plain(plain_), cache(b.nodes.size() * 2, -2),
          affine(b.nodes.size() * 2, UINT32_MAX), deadline(end) {
        cache[0] = 0;
        cache[1] = -1;
        affine[0] = 0;
        affine[1] = 1u << 20;
        for (size_t j = 1; j < b.nodes.size(); ++j) {
            auto node = b.nodes[j];
            uint32_t l = affine[node.lo], h = affine[node.hi];
            if (l != UINT32_MAX && h != UINT32_MAX && (l ^ h) == (1u << 20)) {
                affine[2 * j] = l ^ (1u << node.var);
                affine[2 * j + 1] = affine[2 * j] ^ (1u << 20);
            }
        }
    }
    int map(int lit) {
        if (cache[lit] != -2)
            return cache[lit];
        if (!plain && weight < 8 && cache[lit ^ 1] != -2)
            return cache[lit] = inv(cache[lit ^ 1]);
        if (!plain && affine[lit] != UINT32_MAX)
            return cache[lit] = map_affine(affine[lit]);
        auto node = bdd.nodes[lit / 2];
        int low = node.lo ^ (lit & 1), high = node.hi ^ (lit & 1), s = node.var + 1, v;
        // Map constants lazily and recognize literal functions without gates.
        if (low == 0 && high == 1)
            return cache[lit] = s;
        if (low == 1 && high == 0)
            return cache[lit] = inv(s);
        int lo = map(low), hi = map(high);
        if (low == 0)
            v = combine(1, s, hi);
        else if (high == 1)
            v = combine(2, s, lo);
        else if (low == 1)
            v = combine(2, inv(s), hi);
        else if (high == 0)
            v = combine(1, inv(s), lo);
        else if (plain)
            v = mux_form(s, lo, hi, false);
        else {
            bool up = bdd.implies(low, high), down = bdd.implies(high, low);
            // Expand two decision levels to a balanced four-way Shannon cover.
            // Map grandchildren before speculative rollback, so cached IDs remain valid.
            struct Term {
                int root_side, var, side, value;
            };
            std::vector<Term> terms;
            if (weight >= 8) {
                for (int side = 0; side < 2; ++side) {
                    int child = side ? high : low;
                    if (child < 2) {
                        terms.push_back({side, -1, 0, map(child)});
                        continue;
                    }
                    auto g = bdd.nodes[child / 2];
                    terms.push_back({side, g.var, 0, map(g.lo ^ (child & 1))});
                    terms.push_back({side, g.var, 1, map(g.hi ^ (child & 1))});
                }
            }
            auto form = [&](int which) {
                if (which == 0)
                    return mux_form(s, lo, hi, pos);
                if (which == 1)
                    return mux_form(s, lo, hi, !pos);
                if (which == 2)
                    return combine(2, lo, combine(1, s, hi));
                if (which == 3)
                    return combine(1, hi, combine(2, s, lo));
                if (which == 4)
                    return combine(2, hi, combine(1, inv(s), lo));
                if (which == 5)
                    return combine(1, lo, combine(2, inv(s), hi));
                using Item = std::pair<long long, int>;
                std::priority_queue<Item, std::vector<Item>, std::greater<Item>> q;
                for (auto t : terms) {
                    int cube = t.root_side ? s : inv(s);
                    if (t.var >= 0)
                        cube = combine(1, cube, t.side ? t.var + 1 : inv(t.var + 1));
                    int term = combine(1, cube, t.value);
                    q.emplace(at(term), term);
                }
                while (q.size() > 1) {
                    int a = q.top().second;
                    q.pop();
                    int b = q.top().second;
                    q.pop();
                    int z = combine(2, a, b);
                    q.emplace(at(z), z);
                }
                return q.top().second;
            };
            size_t mark = gates.size();
            int chosen = 0;
            double score = 1e300;
            for (int k = 0; k < 7; ++k) {
                if (k >= 2 && k <= 3 && !up)
                    continue;
                if (k >= 4 && k <= 5 && !down)
                    continue;
                if (k == 6 && terms.empty())
                    continue;
                int root = form(k);
                double value = local_score(root, mark);
                rollback(mark);
                if (value + 1e-9 < score) {
                    score = value;
                    chosen = k;
                }
            }
            v = form(chosen);
        }
        return cache[lit] = v;
    }
    Net finish(const std::vector<int> &roots) {
        std::vector<int> outputs;
        for (int r : roots)
            outputs.push_back(map(r));
        std::vector<unsigned char> used(gates.size());
        std::vector<int> stack = outputs;
        while (!stack.empty()) {
            int v = stack.back();
            stack.pop_back();
            if (v <= c.n)
                continue;
            size_t j = v - c.n - 1;
            if (used[j])
                continue;
            used[j] = 1;
            stack.push_back(gates[j].a);
            if (gates[j].op)
                stack.push_back(gates[j].b);
        }
        Net net;
        net.n = c.n;
        std::vector<int> ids(gates.size() + c.n + 1);
        std::iota(ids.begin(), ids.begin() + c.n + 1, 0);
        for (size_t j = 0; j < gates.size(); ++j)
            if (used[j]) {
                auto g = gates[j];
                ids[c.n + 1 + j] = c.n + 1 + int(net.gates.size());
                g.a = ids[g.a];
                if (g.op)
                    g.b = ids[g.b];
                net.area += c.area[g.op];
                net.gates.push_back(g);
            }
        for (int v : outputs) {
            net.outputs.push_back(v == -1 ? -1 : ids[v]);
            net.delay = std::max(net.delay, at(v));
        }
        if (std::find(outputs.begin(), outputs.end(), -1) != outputs.end())
            net.area += c.area[0];
        return net;
    }
};
// Reassociate single-fanout AND/OR cones using earliest-arrival pairing.
// No truth-table assumptions: associativity and idempotence justify the rewrite.
Net balance(const Net &in, const Case &c) {
    std::vector<int> uses(in.n + 1 + in.gates.size());
    for (const auto &g : in.gates) {
        ++uses[g.a];
        if (g.op)
            ++uses[g.b];
    }
    for (int v : in.outputs)
        if (v >= 0)
            ++uses[v];
    Net out;
    out.n = in.n;
    std::unordered_map<Key, int, Hash> unique;
    std::vector<int> mapped(uses.size());
    std::iota(mapped.begin(), mapped.begin() + in.n + 1, 0);
    auto arrival = [&](int v) {
        return v == -1 ? c.delay[0] : v <= in.n ? 0LL : out.gates[v - in.n - 1].arrival;
    };
    auto gate = [&](int op, int a, int b) {
        if (op && a == b)
            return a;
        if (op && a > b)
            std::swap(a, b);
        Key key{op, a, b};
        auto it = unique.find(key);
        if (it != unique.end())
            return it->second;
        long long t = std::max(arrival(a), op ? arrival(b) : 0LL) + c.delay[op];
        int id = in.n + 1 + int(out.gates.size());
        out.gates.push_back({op, a, b, t});
        unique.emplace(key, id);
        return id;
    };
    for (size_t j = 0; j < in.gates.size(); ++j) {
        const auto &g = in.gates[j];
        int id = in.n + 1 + int(j);
        if (!g.op) {
            mapped[id] = gate(0, mapped[g.a], 0);
            continue;
        }
        std::vector<int> stack{g.a, g.b}, leaves;
        while (!stack.empty()) {
            int v = stack.back();
            stack.pop_back();
            if (v > in.n && uses[v] == 1 && in.gates[v - in.n - 1].op == g.op) {
                const auto child = in.gates[v - in.n - 1];
                stack.push_back(child.a);
                stack.push_back(child.b);
            } else
                leaves.push_back(mapped[v]);
        }
        std::sort(leaves.begin(), leaves.end());
        leaves.erase(std::unique(leaves.begin(), leaves.end()), leaves.end());
        using Item = std::pair<long long, int>;
        std::priority_queue<Item, std::vector<Item>, std::greater<Item>> queue;
        for (int v : leaves)
            queue.emplace(arrival(v), v);
        while (queue.size() > 1) {
            int a = queue.top().second;
            queue.pop();
            int b = queue.top().second;
            queue.pop();
            int v = gate(g.op, a, b);
            queue.emplace(arrival(v), v);
        }
        mapped[id] = queue.top().second;
    }
    for (int v : in.outputs)
        out.outputs.push_back(v == -1 ? -1 : mapped[v]);
    // Remove dead gates created while expanding a subsequently flattened parent.
    std::vector<unsigned char> live(out.gates.size());
    std::vector<int> stack = out.outputs;
    while (!stack.empty()) {
        int v = stack.back();
        stack.pop_back();
        if (v <= in.n)
            continue;
        size_t j = v - in.n - 1;
        if (live[j])
            continue;
        live[j] = 1;
        stack.push_back(out.gates[j].a);
        if (out.gates[j].op)
            stack.push_back(out.gates[j].b);
    }
    Net clean;
    clean.n = in.n;
    std::vector<int> ids(in.n + 1 + out.gates.size());
    std::iota(ids.begin(), ids.begin() + in.n + 1, 0);
    for (size_t j = 0; j < out.gates.size(); ++j)
        if (live[j]) {
            auto g = out.gates[j];
            ids[in.n + 1 + j] = in.n + 1 + int(clean.gates.size());
            g.a = ids[g.a];
            if (g.op)
                g.b = ids[g.b];
            clean.gates.push_back(g);
            clean.area += c.area[g.op];
        }
    for (int v : out.outputs) {
        clean.outputs.push_back(v == -1 ? -1 : ids[v]);
        clean.delay = std::max(clean.delay, arrival(v));
    }
    if (std::find(clean.outputs.begin(), clean.outputs.end(), -1) != clean.outputs.end())
        clean.area += c.area[0];
    return clean;
}

int main(int argc, char **argv) {
    try {
        auto start = Clock::now();
        if (argc != 3)
            throw std::runtime_error("usage: synth INPUT OUTPUT");
        Case c(argv[1]);
        // Search is deterministic in order; deadline truncates it on constrained cases.
        double budget_ms = c.limit > 0 ? std::min(1500.0, 0.60 * double(c.limit)) : 1500.0;
        auto deadline = start + std::chrono::microseconds(static_cast<long long>(budget_ms * 1000));
        std::vector<int> order(c.n);
        std::iota(order.begin(), order.end(), 0);
        std::mt19937 random(2106);
        std::vector<long long> influence(c.n);
        size_t rows = size_t(1) << c.n;
        size_t samples = std::min(rows, size_t(4096));
        for (size_t j = 0; j < samples; ++j) {
            size_t row = samples == rows ? j : size_t(random()) % rows;
            for (int v = 0; v < c.n; ++v) {
                size_t other = row ^ (size_t(1) << (c.n - 1 - v));
                for (int k = 0; k < c.m; ++k)
                    influence[v] += c.table[row * c.m + k] != c.table[other * c.m + k];
            }
        }
        std::vector<int> descending = order;
        std::stable_sort(descending.begin(), descending.end(),
                         [&](int a, int b) { return influence[a] > influence[b]; });
        std::vector<int> best_order = order;
        Net best;
        bool have = false;
        size_t completed = 0;
        auto rank = [&](const Net &x) {
            return std::tuple<long long, long long, long long>(std::max(0LL, x.delay - c.target),
                                                               x.area, x.delay);
        };
        auto map_order = [&](const std::vector<int> &ord, bool plain_first, Clock::time_point end) {
            BDD bdd(c, ord, plain_first ? Clock::time_point::max() : end);
            std::vector<int> roots;
            for (int j = 0; j < c.m; ++j)
                roots.push_back(bdd.build(0, 0, j));
            if (plain_first) {
                Mapper mapper(c, bdd, 0, false, true);
                best = mapper.finish(roots);
                Net balanced = balance(best, c);
                if (rank(balanced) < rank(best))
                    best = std::move(balanced);
                have = true;
                ++completed;
                return;
            }
            for (double w : {16.0, 4.0, 1.0, 0.0}) {
                if (Clock::now() > end)
                    break;
                Mapper mapper(c, bdd, w, false, false, end);
                Net candidate = mapper.finish(roots);
                ++completed;
                Net balanced = balance(candidate, c);
                if (rank(balanced) < rank(candidate))
                    candidate = std::move(balanced);
                if (!have || rank(candidate) < rank(best)) {
                    best = std::move(candidate);
                    best_order = ord;
                    have = true;
                }
            }
        };
        // A first candidate is always completed. Natural order then gets the
        // same optimized mapping pass that earlier versions applied directly.
        map_order(order, true, Clock::time_point::max());
        best_order = order;
        if (Clock::now() < deadline) {
            try {
                map_order(order, false, deadline);
            } catch (const Timeout &) {
            }
        }

        // Stage 2 order screening: generate a wider pool, rank by BDD node count,
        // and spend the remaining budget mapping only the most promising orders.
        std::vector<std::vector<int>> pool{order, descending};
        {
            std::vector<int> reversed = descending;
            std::reverse(reversed.begin(), reversed.end());
            pool.push_back(std::move(reversed));
        }
        {
            std::vector<int> reversed = order;
            std::reverse(reversed.begin(), reversed.end());
            pool.push_back(std::move(reversed));
        }
        for (int k = 0; k < 20; ++k) {
            std::vector<int> candidate = order;
            std::shuffle(candidate.begin(), candidate.end(), random);
            pool.push_back(std::move(candidate));
        }
        for (int k = 0; k < 24; ++k) {
            std::vector<int> candidate = (k % 2) ? descending : best_order;
            int a = int(random() % c.n), b = int(random() % c.n);
            int var = candidate[a];
            candidate.erase(candidate.begin() + a);
            candidate.insert(candidate.begin() + b, var);
            pool.push_back(std::move(candidate));
        }
        std::vector<std::vector<int>> unique;
        for (const auto &candidate : pool)
            if (std::find(unique.begin(), unique.end(), candidate) == unique.end())
                unique.push_back(candidate);
        pool.swap(unique);

        // Stage 1 portfolio phase: keep the original order-search sequence so a
        // wider Stage 2 search cannot accidentally discard its best circuit.
        std::vector<int> stage_order = order;
        std::vector<std::vector<int>> already{order};
        auto stage1_deadline = Clock::now();
        if (stage1_deadline < deadline) {
            double left = std::chrono::duration<double, std::milli>(deadline - stage1_deadline).count();
            stage1_deadline += std::chrono::microseconds(static_cast<long long>(0.0 * left * 1000));
        }
        for (int trial = 1; trial < 32; ++trial) {
            if (Clock::now() > stage1_deadline)
                break;
            if (trial == 1)
                stage_order = descending;
            else if (trial == 2) {
                stage_order = descending;
                std::reverse(stage_order.begin(), stage_order.end());
            } else if (trial == 3) {
                stage_order = order;
                std::reverse(stage_order.begin(), stage_order.end());
            } else if (trial % 3 != 0) {
                stage_order = best_order;
                int a = int(random() % c.n), b = int(random() % c.n);
                int var = stage_order[a];
                stage_order.erase(stage_order.begin() + a);
                stage_order.insert(stage_order.begin() + b, var);
            } else {
                stage_order = order;
                std::shuffle(stage_order.begin(), stage_order.end(), random);
            }
            try {
                map_order(stage_order, false, stage1_deadline);
                if (std::find(already.begin(), already.end(), stage_order) == already.end())
                    already.push_back(stage_order);
            } catch (const Timeout &) {
                break;
            }
        }

        std::vector<std::pair<size_t, int>> scored;
        auto now = Clock::now();
        if (now < deadline) {
            double left = std::chrono::duration<double, std::milli>(deadline - now).count();
            auto screening_deadline =
                now + std::chrono::microseconds(static_cast<long long>(0.35 * left * 1000));
            for (size_t i = 0; i < pool.size(); ++i) {
                if (Clock::now() > screening_deadline)
                    break;
                try {
                    BDD screen_bdd(c, pool[i], screening_deadline);
                    std::vector<int> roots;
                    for (int j = 0; j < c.m; ++j)
                        roots.push_back(screen_bdd.build(0, 0, j));
                    scored.emplace_back(screen_bdd.nodes.size(), int(i));
                } catch (const Timeout &) {
                    break;
                }
            }
        }
        std::sort(scored.begin(), scored.end());
        size_t mapped_orders = 0;
        for (const auto &entry : scored) {
            if (mapped_orders >= 4 || Clock::now() > deadline)
                break;
            const auto &candidate = pool[entry.second];
            if (std::find(already.begin(), already.end(), candidate) != already.end())
                continue;
            ++mapped_orders;
            try {
                map_order(candidate, false, deadline);
            } catch (const Timeout &) {
                break;
            }
        }
        if (!have)
            throw std::runtime_error("no circuit constructed");
        best.write(argv[2]);
        std::cerr << "candidates=" << completed << " area=" << best.area << " delay=" << best.delay
                  << " target=" << c.target << '\n';
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}
