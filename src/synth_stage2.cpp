// AI-assisted experimental implementation: see docs/AI_WORK_LOG.md.
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <numeric>
#include <new>
#include <queue>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unistd.h>
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
// Each word holds 64 consecutive assignments for one output.
struct TruthTable {
    size_t column_words;
    std::vector<uint64_t> words;
    std::vector<uint32_t> support;
    std::vector<long long> influence;

    void resize(size_t rows, int outputs) {
        column_words = (rows + 63) / 64;
        words.resize(column_words * outputs);
        support.resize(outputs);
    }
    int bit(size_t row, int output) const {
        return (words[output * column_words + row / 64] >> (row % 64)) & 1;
    }
    size_t changed_pairs(size_t output, size_t stride) const {
        const auto *col = words.data() + output * column_words;
        size_t changes = 0;
        if (stride < 64) {
            // Select the lower member of each pair within a word.
            uint64_t lower = UINT64_MAX / ((uint64_t(1) << stride) + 1);
            for (size_t i = 0; i < column_words; ++i)
                changes += __builtin_popcountll((col[i] ^ (col[i] >> stride)) & lower);
        } else {
            size_t span = stride / 64;
            for (size_t base = 0; base < column_words; base += 2 * span)
                for (size_t i = 0; i < span; ++i)
                    changes += __builtin_popcountll(col[base+i] ^ col[base+span+i]);
        }
        return changes;
    }
    void analyze(int n) {
        influence.resize(n);
        for (int v = 0; v < n; ++v) {
            const size_t stride = size_t(1) << (n - 1 - v);
            for (size_t j = 0; j < support.size(); ++j) {
                const size_t changes = changed_pairs(j, stride);
                if (changes) support[j] |= uint32_t(1) << v;
                influence[v] += changes;
            }
        }
    }
};

struct Case {
    int n, m;
    TruthTable table;
    long long area[3], delay[3], target, limit = 0, ram = 0;
    explicit Case(const char *path) {
        std::ifstream in(path);
        if (!(in >> n >> m) || n < 1 || n > 20 || m < 1)
            throw std::runtime_error("invalid dimensions");
        size_t rows = size_t(1) << n;
        table.resize(rows, m);
        std::string bits;
        for (size_t i = 0; i < rows; ++i) {
            if (!(in >> bits) || bits.size() != size_t(m))
                throw std::runtime_error("invalid table length");
            for (int j = 0; j < m; ++j) {
                if (bits[j] != '0' && bits[j] != '1')
                    throw std::runtime_error("invalid bit");
                if (bits[j] == '1')
                    table.words[j * table.column_words + i / 64] |= uint64_t(1) << (i % 64);
            }
        }
        table.analyze(n);
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
// Limit optional search using current RSS, with 25% headroom for finalization.
// ru_maxrss can include the parent evaluator's peak across fork/exec.
struct SearchMemory {
    long long limit_kib = 0;
    bool stopped = false;
    bool available(size_t extra_bytes = 0) {
        if (stopped)
            return false;
        if (!limit_kib)
            return true;
        std::ifstream statm("/proc/self/statm");
        unsigned long long virtual_pages = 0, resident_pages = 0;
        static const long page_bytes = sysconf(_SC_PAGESIZE);
        const auto budget_kib = static_cast<unsigned long long>(limit_kib - limit_kib / 4);
        const auto needed_kib = (extra_bytes + 1023) / 1024;
        if (!(statm >> virtual_pages >> resident_pages) || page_bytes <= 0 ||
            needed_kib >= budget_kib || resident_pages * (page_bytes / 1024) >= budget_kib - needed_kib)
            stopped = true;
        return !stopped;
    }
    void check() {
        if (!available())
            throw Timeout{};
    }
} search_memory;
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
  private:
    int two_var_literals[16]{}; // Literal + 1 for each possible four-row truth table.
    int branch(int var, int lo, int hi) {
        if (lo == hi)
            return lo;
        int flip = hi & 1;
        lo ^= flip;
        hi ^= flip; // normalized high edge is regular
        Key key{var, lo, hi};
        auto it = unique.find(key);
        if (it != unique.end())
            return it->second ^ flip;
        int lit = int(nodes.size()) * 2;
        nodes.push_back({var, lo, hi});
        unique.emplace(key, lit);
        return lit ^ flip;
    }

  public:
    int build(int depth, size_t row, int output) {
        if ((++calls & 8191) == 0 && deadline != Clock::time_point::max()) {
            search_memory.check();
            if (Clock::now() > deadline)
                throw Timeout{};
        }
        while (depth < c.n && !(c.table.support[output] & (uint32_t(1) << order[depth])))
            ++depth;
        if (depth == c.n)
            return c.table.bit(row, output);
        const int var = order[depth];
        const size_t bit = size_t(1) << (c.n - 1 - var);
        if (depth + 2 == c.n) {
            const int last = order[depth + 1];
            const size_t low = size_t(1) << (c.n - 1 - last);
            auto at = [&](size_t offset) { return c.table.bit(row | offset, output); };
            int f00 = at(0), f01 = at(low), f10 = at(bit), f11 = at(bit | low);
            int &entry = two_var_literals[f00 | (f01 << 1) | (f10 << 2) | (f11 << 3)];
            if (!entry) {
                int lo = branch(last, f00, f01);
                int hi = branch(last, f10, f11);
                entry = 1 + branch(var, lo, hi);
            }
            return entry - 1;
        }
        int lo = build(depth + 1, row, output);
        int hi = build(depth + 1, row | bit, output);
        return branch(var, lo, hi);
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
    void write(std::ostream &out) const {
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
        if ((++calls & 1023) == 0 && deadline != Clock::time_point::max()) {
            search_memory.check();
            if (Clock::now() > deadline)
                throw Timeout{};
        }
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
        const double timing = weight * double(at(root));
        const int first = c.n + 1 + static_cast<int>(start);
        if (root < first) return timing;
        if (root - first < 64) {
            uint64_t live = uint64_t(1) << (root - first);
            while (live) {
                int bit = 63 - __builtin_clzll(live);
                live ^= uint64_t(1) << bit;
                const Gate &g = gates[start + bit];
                area += c.area[g.op];
                if (g.a >= first) live |= uint64_t(1) << (g.a - first);
                if (g.op && g.b >= first) live |= uint64_t(1) << (g.b - first);
            }
            return double(area) + timing;
        }
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
        return double(area) + timing;
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
        if (os + 1e-9 < ds) return other;
        rollback(mark);
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
        if (second < first) return v;
        rollback(mark);
        return form(false);
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

#include "round51/synthesis.hpp"
#include "round51/shallow_table.hpp"

static bool round51_supports(const Case &c) {
    // The research mapper uses int arrival times and prices.
    const auto max_cost = std::max(*std::max_element(c.area, c.area + 3),
                                   *std::max_element(c.delay, c.delay + 3));
    return max_cost <= INT_MAX / (8LL * c.n + 8) && c.target < 1000000000;
}

static r51::Graph typed_net(const Net &net) {
    r51::Graph graph(net.n, int(net.outputs.size()));
    const bool one = std::find(net.outputs.begin(), net.outputs.end(), -1) != net.outputs.end();
    auto id = [&](int v) { return v == -1 ? net.n + 1 : v + (one && v > net.n); };
    graph.nodes.reserve(graph.nodes.size() + net.gates.size() + one);
    if (one) graph.nodes.push_back({2, 0, 0});
    for (const auto &g : net.gates)
        graph.nodes.push_back({(g.op + 2) % 3, id(g.a), g.op ? id(g.b) : 0});
    for (int v : net.outputs) graph.outputs.push_back(id(v));
    return graph;
}

// Both graphs implement the same output vector. Switch one output at a time,
// charging a gate only while a live output or live parent still references it.
static r51::Graph mix_outputs(const Case &c, const Net &best, const r51::Graph &trial,
                              const r51::Costs &costs, Clock::time_point end) {
    const size_t capacity = best.gates.size() + trial.nodes.size();
    if (c.m < 2 || Clock::now() >= end || !search_memory.available(capacity * 96))
        return {};
    r51::Builder joined(trial);
    joined.intern.reserve(capacity);
    std::vector<int> ids(c.n + 1 + best.gates.size());
    std::iota(ids.begin(), ids.begin() + c.n + 1, 0);
    auto signal = [&](int v) { return v == -1 ? joined.add(2, 0, 0, costs) : ids[v]; };
    for (size_t i = 0; i < best.gates.size(); ++i) {
        if ((i & 4095) == 0 && Clock::now() >= end) return {};
        auto g = best.gates[i];
        ids[c.n + 1 + i] = joined.add((g.op + 2) % 3, signal(g.a), g.op ? signal(g.b) : 0, costs);
    }
    std::vector<int> chosen;
    chosen.reserve(c.m);
    for (int v : best.outputs) chosen.push_back(signal(v));
    ids.resize(trial.nodes.size());
    for (size_t i = c.n + 1; i < trial.nodes.size(); ++i) {
        if ((i & 4095) == 0 && Clock::now() >= end) return {};
        auto g = trial.nodes[i];
        ids[i] = joined.add(g.op, ids[g.a], g.op == 2 ? 0 : ids[g.b], costs);
    }

    const auto &nodes = joined.graph.nodes;
    std::vector<int> uses(nodes.size()), stack;
    const int price[] = {costs.aa, costs.ao, costs.an};
    long long area = 0;
    auto change_use = [&](int root, int step) {
        stack.push_back(root);
        while (!stack.empty()) {
            int id = stack.back();
            stack.pop_back();
            if (id <= c.n) continue;
            uses[id] += step;
            if (uses[id] != (step > 0 ? 1 : 0)) continue;
            const auto &g = nodes[id];
            area += step * static_cast<long long>(price[g.op]);
            stack.push_back(g.a);
            if (g.op != 2) stack.push_back(g.b);
        }
    };
    std::multiset<int> delays;
    for (int root : chosen) {
        change_use(root, 1);
        delays.insert(joined.ready[root]);
    }
    bool changed = false;
    for (int j = 0; j < c.m; ++j) {
        if (Clock::now() >= end) break;
        int old = chosen[j], next = ids[trial.outputs[j]];
        if (old == next) continue;
        int old_delay = joined.ready[old], new_delay = joined.ready[next];
        int previous_max = *delays.rbegin();
        long long previous_area = area;
        delays.erase(delays.find(old_delay));
        int other_max = delays.empty() ? 0 : *delays.rbegin();
        change_use(old, -1);
        change_use(next, 1);
        auto score = [&](int delay, long long cost, int local_delay) {
            return std::make_tuple(std::max(0LL, delay - c.target), cost, delay, local_delay);
        };
        if (score(std::max(other_max, new_delay), area, new_delay) <
            score(previous_max, previous_area, old_delay)) {
            chosen[j] = next;
            changed = true;
        } else {
            change_use(next, -1);
            change_use(old, 1);
        }
        delays.insert(joined.ready[chosen[j]]);
    }
    return changed ? joined.finish(std::move(chosen)) : r51::Graph{};
}

// Compare completed Round 5.1 circuits in the same units as the BDD search.
static void search_round51(const Case &c, Clock::time_point end, Net &best, size_t &completed) {
    if (!search_memory.available() || Clock::now() >= end) return;
    round51::Truth data{c.n, c.m, int(c.area[1]), int(c.delay[1]), int(c.area[2]),
                        int(c.delay[2]), int(c.area[0]), int(c.delay[0]), c.table, c.table.influence};
    r51::Costs costs{data.aa, data.da, data.ao, data.doo, data.an, data.dn};
    auto rank = [&](long long area, long long delay) {
        return std::make_tuple(std::max(0LL, delay - c.target), area, delay);
    };
    auto consider = [&](const r51::Graph &g) {
        auto score = g.metric(costs);
        if (!score.valid) return;
        ++completed;
        if (rank(score.area, score.delay) >= rank(best.area, best.delay)) return;
        Net candidate{};
        candidate.n = c.n;
        candidate.area = score.area;
        candidate.delay = score.delay;
        candidate.outputs = g.outputs;
        for (size_t id = c.n + 1; id < g.nodes.size(); ++id) {
            auto gate = g.nodes[id];
            candidate.gates.push_back({(gate.op + 1) % 3, gate.a, gate.b, 0});
        }
        best = std::move(candidate);
    };
    if (best.delay > c.target) {
        // Keep time for the variable-order portfolio even on a dense table.
        auto now = Clock::now();
        consider(shallow_table(c, costs, now + (end - now) / 2));
    }
    auto refine_best = [&]() {
        if (Clock::now() >= end || !search_memory.available()) return;
        auto base = typed_net(best);
        consider(round4_algebra::rewrite(base, costs));
        if (Clock::now() >= end) return;
        round4_phase::Engine phase(base, costs);
        for (int polarity : {0, 1}) {
            if (Clock::now() >= end || !search_memory.available()) break;
            if (polarity) {
                std::fill(phase.phase.begin() + c.n + 1, phase.phase.end(), 1);
                phase.improve(4);
            }
            auto mapped = phase.construct();
            consider(mapped);
            if (Clock::now() < end)
                consider(round4_algebra::rewrite(mapped, costs));
        }
    };
    // Natural order already has a mapped circuit. Compare the opposite
    // order and polish the winner before spending time on similar orders.
    auto last_refined = rank(best.area, best.delay);
    auto local_area = round51::Opt{"output-influence",false,true,false,1,0,true};
    local_area.area_first = true;
    auto regular = round51::Opt{"influence-desc",false,true,true,1,0};
    regular.regular = true;
    auto regular_reverse = regular;
    regular_reverse.order = "reverse";
    auto regular_local = regular;
    regular_local.order = "output-influence";
    auto decoder = local_area;
    decoder.decoder = true;
    auto reverse_decoder = decoder;
    reverse_decoder.order = "reverse";
    for (auto option : {round51::Opt{"reverse",true,false,true,1,0,true},
                        local_area, regular, decoder, reverse_decoder, regular_reverse, regular_local,
                        round51::Opt{"influence-desc",true,false,true,1,0,true},
                        round51::Opt{"influence-desc",false,true,true,1,0,true},
                        round51::Opt{"natural",true,false,true,1,0,false},
                        round51::Opt{"rotate",true,false,true,1,0,true},
                        round51::Opt{"influence-asc",true,false,true,1,0,true}}) {
        if (Clock::now() >= end || !search_memory.available()) break;
        auto graph = round51::run_synth(data, option, end);
        auto score = graph.metric(costs);
        auto keep = [&](r51::Graph next) {
            auto value = next.metric(costs);
            if (value.valid && rank(value.area, value.delay) < rank(score.area, score.delay)) {
                graph = std::move(next);
                score = value;
            }
        };
        if (Clock::now() < end) keep(r51::rebalance(graph, costs));
        if (Clock::now() < end) keep(r51::area_recover(graph, costs, int(c.target), 0));
        auto mixed = mix_outputs(c, best, graph, costs, end);
        consider(graph);
        if (mixed.valid) consider(mixed);
        if (option.order == "reverse") {
            refine_best();
            last_refined = rank(best.area, best.delay);
        }
    }
    if (rank(best.area, best.delay) < last_refined) refine_best();
}

int main(int argc, char **argv) {
    try {
        auto start = Clock::now();
        if (argc != 3)
            throw std::runtime_error("usage: synth INPUT OUTPUT");
        Case c(argv[1]);
        search_memory.limit_kib = c.ram;
        // Search is deterministic in order; deadline truncates it on constrained cases.
        double budget_ms = c.limit > 0 ? std::min(1800.0, 0.45 * double(c.limit)) : 1800.0;
        auto end = start + std::chrono::microseconds(static_cast<long long>(budget_ms * 1000));
        const bool use_round51 = round51_supports(c);
        auto deadline = use_round51 ? start + (end - start) / 2 : end;
        std::vector<int> order(c.n);
        std::iota(order.begin(), order.end(), 0);
        std::mt19937 random(2106);
        std::vector<int> descending = order;
        std::stable_sort(descending.begin(), descending.end(),
                         [&](int a, int b) { return c.table.influence[a] > c.table.influence[b]; });
        std::vector<int> best_order = order;
        Net best;
        size_t completed = 0;
        std::vector<std::pair<std::tuple<long long, long long, long long>, std::vector<int>>> measured;
        auto rank = [&](const Net &x) {
            return std::tuple<long long, long long, long long>(std::max(0LL, x.delay - c.target),
                                                               x.area, x.delay);
        };
        auto map_order = [&](const std::vector<int> &ord, bool plain_first, Clock::time_point end,
                             const std::vector<double> &weights, bool stop_at_target = false) {
            if (!plain_first) search_memory.check();
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
                ++completed;
                return;
            }
            for (double w : weights) {
                if (Clock::now() > end)
                    break;
                search_memory.check();
                Mapper mapper(c, bdd, w, false, false, end);
                Net candidate = mapper.finish(roots);
                ++completed;
                Net balanced = balance(candidate, c);
                if (rank(balanced) < rank(candidate))
                    candidate = std::move(balanced);
                bool meets_target = candidate.delay <= c.target;
                auto measured_it = std::find_if(measured.begin(), measured.end(),
                    [&](const auto &entry) { return entry.second == ord; });
                if (measured_it == measured.end())
                    measured.emplace_back(rank(candidate), ord);
                else if (rank(candidate) < measured_it->first)
                    measured_it->first = rank(candidate);
                if (rank(candidate) < rank(best)) {
                    best = std::move(candidate);
                    best_order = ord;
                }
                if (stop_at_target && meets_target) break;
            }
        };
        // Complete the fallback before starting optional search.
        map_order(order, true, Clock::time_point::max(), {});
        auto write_best = [&]() {
            std::ofstream output(argv[2]);
            best.write(output);
            output.close();
            if (!output) throw std::runtime_error("output write failed");
            std::cerr << "candidates=" << completed << " area=" << best.area << " delay=" << best.delay
                      << " target=" << c.target << " ram_search_stopped=" << search_memory.stopped << '\n';
        };
        if (best.area == 0 && best.delay == 0) {
            write_best();
            return 0;
        }
        auto try_map_order = [&](const std::vector<int> &ord, Clock::time_point end,
                                 const std::vector<double> &weights, bool stop_at_target = false) {
            try {
                map_order(ord, false, end, weights, stop_at_target);
                return true;
            } catch (const Timeout &) {
            } catch (const std::bad_alloc &) {
                search_memory.stopped = true;
            }
            return false;
        };
        if (descending != order && Clock::now() < deadline)
            try_map_order(descending, deadline, {1.0, 16.0}, true);
        // Sampling can separate nearly tied influence counts differently.
        // Compare that order on the complete table before sweeping more weights.
        const size_t rows = size_t(1) << c.n;
        if (rows > 4096 && Clock::now() < deadline && search_memory.available()) {
            std::vector<long long> sampled(c.n);
            for (int j = 0; j < 4096; ++j) {
                const size_t row = random() % rows;
                for (int v = 0; v < c.n; ++v) {
                    const size_t other = row ^ (size_t(1) << (c.n - 1 - v));
                    for (int k = 0; k < c.m; ++k)
                        sampled[v] += c.table.bit(row, k) != c.table.bit(other, k);
                }
            }
            auto sampled_order = order;
            std::stable_sort(sampled_order.begin(), sampled_order.end(),
                             [&](int a, int b) { return sampled[a] > sampled[b]; });
            if (sampled_order != descending && sampled_order != order)
                try_map_order(sampled_order, deadline, {1.0}, true);
        }
        // Compare orders at one weight before refining their area/delay tradeoffs.
        if (Clock::now() < deadline)
            try_map_order(order, deadline, {4.0});

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
        auto shuffle_random = random;
        for (int k = 0; k < 24; ++k) {
            auto shuffled = order;
            std::shuffle(shuffled.begin(), shuffled.end(), shuffle_random);
            pool.push_back(std::move(shuffled));
            std::vector<int> candidate;
            if (k % 3 == 2) {
                candidate = order;
                std::shuffle(candidate.begin(), candidate.end(), random);
            } else {
                candidate = k % 3 == 0 ? best_order : descending;
                int from = int(random() % c.n), to = int(random() % c.n);
                int var = candidate[from];
                candidate.erase(candidate.begin() + from);
                candidate.insert(candidate.begin() + to, var);
            }
            pool.push_back(std::move(candidate));
        }
        std::vector<std::vector<int>> unique;
        for (const auto &candidate : pool)
            if (std::find(unique.begin(), unique.end(), candidate) == unique.end())
                unique.push_back(candidate);
        pool.swap(unique);

        std::vector<std::pair<size_t, int>> scored;
        auto now = Clock::now();
        if (now < deadline && search_memory.available()) {
            double left = std::chrono::duration<double, std::milli>(deadline - now).count();
            auto screening_deadline =
                now + std::chrono::microseconds(static_cast<long long>(0.35 * left * 1000));
            for (size_t i = 0; i < pool.size(); ++i) {
                if (Clock::now() > screening_deadline || !search_memory.available())
                    break;
                try {
                    BDD screen_bdd(c, pool[i], screening_deadline);
                    for (int j = 0; j < c.m; ++j)
                        screen_bdd.build(0, 0, j);
                    scored.emplace_back(screen_bdd.nodes.size(), int(i));
                } catch (const Timeout &) {
                    break;
                } catch (const std::bad_alloc &) {
                    search_memory.stopped = true;
                    break;
                }
            }
        }
        std::sort(scored.begin(), scored.end());
        // Node count proposes a shortlist; actual reachable mapped area/delay
        // decides which orders receive further weights. Give each probe a fair
        // share so a slow first order cannot consume the entire mapping budget.
        std::vector<std::vector<int>> shortlist;
        auto add_order = [&](const std::vector<int> &candidate) {
            if (candidate != order &&
                std::find(shortlist.begin(), shortlist.end(), candidate) == shortlist.end())
                shortlist.push_back(candidate);
        };
        for (const auto &entry : scored) {
            add_order(pool[entry.second]);
            if (shortlist.size() >= 4) break;
        }
        add_order(descending);
        auto reversed = descending;
        std::reverse(reversed.begin(), reversed.end());
        add_order(reversed);
        now = Clock::now();
        auto probe_deadline = now + (deadline - now) * 2 / 3;
        for (size_t i = 0; i < shortlist.size(); ++i) {
            now = Clock::now();
            if (now >= probe_deadline || !search_memory.available()) break;
            auto end = now + (probe_deadline - now) / (shortlist.size() - i);
            try_map_order(shortlist[i], end, {4.0});
        }
        // Copy before refinement: map_order updates measured entries.
        auto refinement = measured;
        std::stable_sort(refinement.begin(), refinement.end(),
                         [](const auto &a, const auto &b) { return a.first < b.first; });
        for (const auto &entry : refinement) {
            if (Clock::now() >= deadline || !search_memory.available()) break;
            if (!try_map_order(entry.second, deadline, {1.0, 0.0, 16.0}))
                break;
        }
        try {
            if (use_round51)
                search_round51(c, end, best, completed);
        } catch (const round51::CandidateTimeout &) {
            // Completed candidates are already in best.
        } catch (const std::bad_alloc &) {
            search_memory.stopped = true;
        }
        write_best();
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}
