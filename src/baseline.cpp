// Transparent AI-assisted baseline. See docs/AI_WORK_LOG.md before assessment use.
// Reduced ordered binary decision diagram, mapped by Shannon expansion.
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

struct Key {
    int kind, a, b;
    bool operator==(const Key &x) const {
        return kind == x.kind && a == x.a && b == x.b;
    }
};
struct Hash {
    size_t operator()(const Key &k) const {
        uint64_t h = static_cast<uint32_t>(k.kind);
        h = (h * 1000003) ^ static_cast<uint32_t>(k.a);
        h = (h * 1000003) ^ static_cast<uint32_t>(k.b);
        h ^= h >> 30;
        h *= 0xbf58476d1ce4e5b9ULL;
        h ^= h >> 27;
        return static_cast<size_t>(h);
    }
};
struct Node {
    int var, lo, hi;
};
class BDD {
    int n, m;
    const std::vector<unsigned char> &table;
    std::unordered_map<Key, int, Hash> unique;

  public:
    std::vector<Node> nodes{{-1, 0, 0}, {-1, 1, 1}};
    BDD(int n_, int m_, const std::vector<unsigned char> &t) : n(n_), m(m_), table(t) {}
    int build(int depth, size_t row, int output) {
        if (depth == n)
            return table[row * static_cast<size_t>(m) + output];
        int lo = build(depth + 1, row, output);
        int hi = build(depth + 1, row | (size_t(1) << (n - 1 - depth)), output);
        if (lo == hi)
            return lo;
        Key k{depth, lo, hi};
        auto it = unique.find(k);
        if (it != unique.end())
            return it->second;
        int id = static_cast<int>(nodes.size());
        nodes.push_back({depth, lo, hi});
        unique.emplace(k, id);
        return id;
    }
};
struct Gate {
    int kind, a, b;
}; // 0=NOT, 1=AND2, 2=OR2
class Circuit {
    int n;
    std::unordered_map<Key, int, Hash> unique;
    std::vector<int> mapped;
    const BDD &bdd;

  public:
    std::vector<Gate> gates;
    Circuit(int n_, const BDD &b) : n(n_), mapped(b.nodes.size(), -1), bdd(b) {
        mapped[0] = 0;
    }
    int gate(int kind, int a, int b = 0) {
        if (kind != 0 && a > b)
            std::swap(a, b);
        Key k{kind, a, b};
        auto it = unique.find(k);
        if (it != unique.end())
            return it->second;
        int id = n + 1 + static_cast<int>(gates.size());
        gates.push_back({kind, a, b});
        unique.emplace(k, id);
        return id;
    }
    int inv(int a) {
        if (a > n && gates[a - n - 1].kind == 0)
            return gates[a - n - 1].a;
        return gate(0, a);
    }
    int map(int u) {
        if (mapped[u] >= 0)
            return mapped[u];
        if (u == 1)
            return mapped[u] = inv(0);
        const auto node = bdd.nodes[u];
        int s = node.var + 1;
        int a = map(node.lo), b = map(node.hi), v;
        // Constant reductions avoid inserting gates whose result is known.
        if (node.lo == 0 && node.hi == 1)
            v = s;
        else if (node.lo == 1 && node.hi == 0)
            v = inv(s);
        else if (node.lo == 0)
            v = gate(1, s, b);
        else if (node.hi == 1)
            v = gate(2, s, a);
        else if (node.lo == 1)
            v = gate(2, inv(s), b);
        else if (node.hi == 0)
            v = gate(1, inv(s), a);
        else
            v = gate(2, gate(1, inv(s), a), gate(1, s, b));
        return mapped[u] = v;
    }
    void write(const std::string &path, const std::vector<int> &outputs) const {
        // Discard all unreferenced gates, including constants mapped speculatively.
        std::vector<unsigned char> used(gates.size());
        std::vector<int> stack = outputs;
        while (!stack.empty()) {
            int u = stack.back();
            stack.pop_back();
            if (u <= n)
                continue;
            size_t j = static_cast<size_t>(u - n - 1);
            if (used[j])
                continue;
            used[j] = 1;
            stack.push_back(gates[j].a);
            if (gates[j].kind)
                stack.push_back(gates[j].b);
        }
        std::vector<int> ids(n + 1 + gates.size());
        for (int i = 0; i <= n; ++i)
            ids[i] = i;
        std::ofstream out(path);
        if (!out)
            throw std::runtime_error("cannot open output");
        int next = n + 1;
        for (size_t i = 0; i < gates.size(); ++i)
            if (used[i]) {
                const auto g = gates[i];
                ids[n + 1 + i] = next;
                out << next++ << ' '
                    << (g.kind == 0   ? "NOT"
                        : g.kind == 1 ? "AND2"
                                      : "OR2")
                    << ' ' << ids[g.a];
                if (g.kind)
                    out << ' ' << ids[g.b];
                out << '\n';
            }
        for (size_t i = 0; i < outputs.size(); ++i)
            out << (i ? " " : "") << ids[outputs[i]];
        out << '\n';
        if (!out)
            throw std::runtime_error("output write failed");
    }
};
int main(int argc, char **argv) {
    try {
        if (argc != 3)
            throw std::runtime_error("usage: baseline INPUT OUTPUT");
        std::ifstream in(argv[1]);
        int n, m;
        if (!(in >> n >> m) || n < 1 || n > 20 || m < 1)
            throw std::runtime_error("invalid dimensions");
        size_t rows = size_t(1) << n;
        std::vector<unsigned char> table(rows * static_cast<size_t>(m));
        std::string word;
        for (size_t i = 0; i < rows; ++i) {
            if (!(in >> word) || word.size() != static_cast<size_t>(m))
                throw std::runtime_error("invalid truth table");
            for (int j = 0; j < m; ++j) {
                if (word[j] != '0' && word[j] != '1')
                    throw std::runtime_error("invalid bit");
                table[i * m + j] = word[j] - '0';
            }
        }
        long long value;
        for (int i = 0; i < 7; ++i)
            if (!(in >> value) || value <= 0)
                throw std::runtime_error("invalid gate cost or target delay");
        BDD bdd(n, m, table);
        std::vector<int> roots;
        for (int j = 0; j < m; ++j)
            roots.push_back(bdd.build(0, 0, j));
        Circuit c(n, bdd);
        std::vector<int> outputs;
        for (int r : roots)
            outputs.push_back(c.map(r));
        c.write(argv[2], outputs);
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}
