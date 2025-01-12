
#include <iostream>
#include <cassert>
#include <set>

#include <omp.h>
#include "satgirgs/Generator.h"


using namespace std;

struct Clustering {
    int fourPaths;
    int fourCycles;
    float closedProbability;
};

Clustering measureClustering(int n, int m, const vector<pair<int, int>> & edges) {
    // n = variables
    // m = clauses
    // edge IDs: (variable_id, n + clause_id)
    auto variable_adj = vector<set<int>>(n);
    auto clause_adj = vector<vector<int>>(m);
    for (auto [u, v] : edges) {
        auto variableId = u;
        auto clauseId = v - n;
        variable_adj[variableId].insert(clauseId);
        clause_adj[clauseId].push_back(variableId);
    }
    int fourCycles = 0;
    int fourPaths = 0;

    for (int clauseId = 0; clauseId < m; ++clauseId) {
        for (int var1 = 0; var1 < clause_adj[clauseId].size(); ++var1) {
            auto var1Id = clause_adj[clauseId][var1];

            fourPaths += (clause_adj[clauseId].size() - 1) * (variable_adj[var1Id].size() - 1);
            for (int var2 = var1 + 1; var2 < clause_adj[clauseId].size(); ++var2) {
                auto var2Id = clause_adj[clauseId][var2];
                // TODO Optimize
                std::vector<int> intersection;
                std::set_intersection(variable_adj[var1Id].begin(), variable_adj[var1Id].end(), variable_adj[var2Id].begin(), variable_adj[var2Id].end(), back_inserter(intersection));
                fourCycles += intersection.size() - 1;
            }
        }
    }
    fourCycles /= 2;
    return {fourPaths, fourCycles, fourCycles * 4.f / fourPaths};
}

template<unsigned int d>
void measure(int n, int m, int k, float t, float ple, int threads, int seed, int plot) {

    omp_set_num_threads(threads);
    assert(threads == omp_get_max_threads());

    auto ncseed = seed+ 10000;
    auto cseed = seed + 10001;
    auto wseed = seed + 10002;
    auto eseed = seed+ 100000;

    auto weights = satgirgs::generateWeights(n, ple, wseed);
    auto nc_positions = satgirgs::generatePositions(n, d, ncseed);
    auto nc_nodes = satgirgs::convertToNodes<d>(nc_positions, weights);
    auto c_positions = satgirgs::generatePositions(m, d, cseed);
    std::vector<double> c_pseudoweights(m, 1); // clause nodes all have weight 1 in the model
    auto c_nodes = satgirgs::convertToNodes<d>(c_positions, c_pseudoweights, nc_nodes.size());

    auto edges = satgirgs::generateEdges<d>(c_nodes, nc_nodes, k, t, eseed, true);
    auto edgeCount = edges.size();
    auto variablesPerClause = edgeCount / m;
    auto clustering = measureClustering(n, m, edges);

    cout << d << ','
         << n << ','
         << m << ','
         << k << ','
         << t << ','
         << ple << ','
         << threads << ','
         << seed << ','
         << plot << ','
         << edgeCount << ','
         << variablesPerClause << ','
         << clustering.fourPaths << ','
         << clustering.fourCycles << ','
         << clustering.closedProbability << endl;
}


int main(int argc, char* argv[]) {

    cout << "d,n,m,k,t,ple,threads,seed,plot,edgeCount,variablesPerClause,fourPaths,fourCycles,closedProbability" << endl;

    int seed = 0;

    auto n = 1000;
    auto m = 4000;
    auto k = 3;
    auto threads = 1;
    auto reps = 100;
    auto ple = 2.5;
    double t = 1.0;

    for (auto d : {1, 2, 3, 4, 5}) {
        clog << "d = " << d << endl;
        for (auto t : {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0}) {
            clog << "t = " << t << endl;
            for (int rep = 0; rep < reps; ++rep) {
                clog << "rep " << rep << endl;

                switch(d) {
                    case 1: measure<1>(n, m, k, t, ple, threads, ++seed, 2); break;
                    case 2: measure<2>(n, m, k, t, ple, threads, ++seed, 2); break;
                    case 3: measure<3>(n, m, k, t, ple, threads, ++seed, 2); break;
                    case 4: measure<4>(n, m, k, t, ple, threads, ++seed, 2); break;
                    case 5: measure<5>(n, m, k, t, ple, threads, ++seed, 2); break;
                    default:break;
                }
            }
        /*
        clog << "growing n" << endl;
        for(int i = 1<<10; i<= (1<<15); i <<= 1) {
            clog << i << endl;
            for(auto deg : {10,15, 20})
		if(deg*4 < i) 
		    measure(i, m, deg, t, threads, ++seed, 0);
        }


        clog << "growing deg" << endl;
        for(int i = 2; i<= 1<<6; i <<= 1) {
            clog << i << endl;
            measure(n, m, i, t, threads, ++seed, 1);
        }*/
        }
    }

    return 0;
}
