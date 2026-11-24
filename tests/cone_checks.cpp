// Exercise individual candidates that a complete portfolio run could discard.
#define main synth_cli
#include "../src/synth_stage2.cpp"
#undef main

static void save(const r51::Graph &graph, const std::string &path) {
    Net net{graph.n, {}, graph.outputs};
    for (size_t i = graph.n + 1; i < graph.nodes.size(); ++i) {
        auto g = graph.nodes[i];
        net.gates.push_back({(g.op + 1) % 3, g.a, g.b, 0});
    }
    std::ofstream out(path);
    net.write(out);
}

int main(int argc, char **argv) {
    if (argc != 3) return 1;
    Case c(argv[1]);
    r51::Costs unit{1, 1, 1, 1, 1, 1};
    // Both circuits produce AND, OR and two copies of constant one.
    // Each has one direct gate and implements the other function by De Morgan.
    Net a{2, {{1,1,2,1},{0,1,0,1},{0,2,0,1},{1,4,5,2},{0,6,0,3}},
          {3,7,-1,-1}, 6, 3};
    r51::Graph b(2,4);
    b.nodes.insert(b.nodes.end(), {{1,1,2},{2,1,0},{2,2,0},{1,4,5},{2,6,0},{2,0,0}});
    b.outputs = {7,3,8,8};
    if (c.n == 2 && c.m == 4) {
        auto faster = critical_cuts(b, unit, 1, Clock::time_point::max());
        if (faster.metric(unit).delay != 1) return 8;
        save(faster, std::string(argv[2]) + "-timing");
        auto hybrid = mix_outputs(c, a, b, unit, Clock::time_point::max());
        auto metrics = hybrid.metric(unit);
        if (!metrics.valid || metrics.area != 3 || metrics.delay != 1) return 2;
        if (hybrid.outputs[2] != hybrid.outputs[3]) return 3;
        save(hybrid, std::string(argv[2]) + "-mixed");
        polish_cuts(c, Clock::now(), a);
        if (a.area >= 6 || a.delay > c.target) return 6;
        std::ofstream polished(std::string(argv[2]) + "-cuts");
        a.write(polished);
    }
    r51::Builder builder(r51::Graph(c.n, c.m));
    CofactorTiming search(builder, unit, Clock::time_point::max());
    std::vector<int> roots;
    for (int j = 0; j < c.m; ++j) roots.push_back(search.output(c, j, int(c.target)));
    save(builder.finish(std::move(roots)), std::string(argv[2]) + "-cofactors");
    for (int op = 0; op < 2; ++op) {
        auto costs = unit;
        if (op == 1) costs.da = 2;
        auto graph = shallow_table(c, costs, Clock::time_point::max());
        if (!graph.valid) return 5;
        if (op == 0) {
            SharedFactors factors(c, graph, costs, Clock::time_point::max());
            auto shared = factors.run(graph);
            if (!shared.valid) return 7;
            save(shared, std::string(argv[2]) + "-factors");
        }
        save(graph, std::string(argv[2]) + "-table" + std::to_string(op));
    }
    for (int expensive : {1, 2}) {
        auto previous = c.area[expensive];
        c.area[expensive] = 3;
        std::ofstream stream(std::string(argv[2]) + "-stream" + std::to_string(expensive));
        StreamingMapper(c, stream).write();
        c.area[expensive] = previous;
    }
    round51::Truth data{c.n,c.m,1,1,1,1,1,1,c.table,c.table.influence};
    round51::Opt opt{"output-influence",false,true,false,1,0,true};
    opt.decoder = true;
    opt.area_first = true;
    for (int regular = 0; regular <= 1; ++regular) {
        opt.regular = regular;
        auto graph = round51::run_synth(data, opt);
        auto metrics = graph.metric(unit);
        if (!metrics.valid) return 4;
        save(graph, std::string(argv[2]) + std::to_string(regular));
    }
}
