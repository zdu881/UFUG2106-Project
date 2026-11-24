// Re-express short gate cones only on paths that miss the working delay target.
#pragma once

static r51::Graph critical_cuts(const r51::Graph &old, const r51::Costs &costs,
                                int target, Clock::time_point end) {
    r51::Builder builder(old);
    std::vector<long long> need(old.nodes.size(), target);
    for (int id = int(old.nodes.size()) - 1; id > old.n; --id) {
        const auto g = old.nodes[id];
        const auto t = need[id] - r84cut::delay(costs, g.op);
        need[g.a] = std::min(need[g.a], t);
        if (g.op != 2) need[g.b] = std::min(need[g.b], t);
    }
    std::vector<int> mapped(old.nodes.size());
    std::iota(mapped.begin(), mapped.begin() + old.n + 1, 0);
    for (int id = old.n + 1; id < int(old.nodes.size()); ++id) {
        const auto g = old.nodes[id];
        int best = builder.add(g.op, mapped[g.a], g.op == 2 ? 0 : mapped[g.b], costs);
        if (builder.ready[best] > need[id] && Clock::now() < end) {
            // Expanding a frontier leaf to its fanins keeps the cut exact.
            std::vector<std::vector<int>> cuts{{id}};
            for (size_t i = 0; i < cuts.size() && i < 20; ++i) {
                const auto cut = cuts[i];
                if (cut.size() >= 2) {
                    const int f = r84cut::truth(old, id, cut);
                    std::vector<int> pins;
                    for (int x : cut) pins.push_back(mapped[x]);
                    r84cut::Env e{builder, costs, pins, int(cut.size())};
                    auto compare = [&](int x) {
                        if (builder.ready[x] < builder.ready[best]) best = x;
                    };
                    const unsigned full = (1u << (1u << cut.size())) - 1u;
                    compare(e.sop(f, 0));
                    compare(e.neg(e.sop(full ^ f, 0)));
                    std::vector<int> order(pins.size());
                    std::iota(order.begin(), order.end(), 0);
                    std::stable_sort(order.begin(), order.end(), [&](int a, int b) {
                        return builder.ready[pins[a]] > builder.ready[pins[b]];
                    });
                    compare(e.build_shannon(f, order));
                }
                if (builder.ready[best] <= need[id] || Clock::now() >= end) break;
                for (int x : cut) {
                    if (x <= old.n) continue;
                    const auto child = old.nodes[x];
                    auto next = cut;
                    next.erase(std::lower_bound(next.begin(), next.end(), x));
                    next.push_back(child.a);
                    if (child.op != 2) next.push_back(child.b);
                    std::sort(next.begin(), next.end());
                    next.erase(std::unique(next.begin(), next.end()),next.end());
                    if (next.size() <= 4 && std::find(cuts.begin(), cuts.end(), next) == cuts.end())
                        cuts.push_back(std::move(next));
                }
            }
        }
        mapped[id] = best;
    }
    std::vector<int> outputs;
    for (int x : old.outputs) outputs.push_back(mapped[x]);
    return builder.finish(std::move(outputs));
}
