#pragma once
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <optional>
#include <type_traits>
#include <fishnet/Graph.hpp>
#include <fishnet/FunctionalConcepts.hpp>
#include <fishnet/Option.hpp>

namespace fishnet::graph::MST {

namespace __impl {

/**
 * @brief Union-Find (Disjoint Set Union) data structure using integer indices.
 * Maps nodes to indices externally, avoiding storing node copies internally.
 */
class UnionFind {
private:
    std::vector<size_t> parent;
    std::vector<size_t> rank;

public:
    explicit UnionFind(size_t n) : parent(n), rank(n, 0) {
        for (size_t i = 0; i < n; ++i) {
            parent[i] = i;
        }
    }

    size_t find(size_t x) {
        if (parent[x] != x) {
            parent[x] = find(parent[x]);
        }
        return parent[x];
    }

    bool unite(size_t a, size_t b) {
        size_t rootA = find(a);
        size_t rootB = find(b);
        if (rootA == rootB) {
            return false;
        }
        if (rank[rootA] < rank[rootB]) {
            std::swap(rootA, rootB);
        }
        parent[rootB] = rootA;
        if (rank[rootA] == rank[rootB]) {
            rank[rootA]++;
        }
        return true;
    }

    bool connected(size_t a, size_t b) {
        return find(a) == find(b);
    }
};
} // namespace __impl

/**
 * @brief Compute the Minimum Spanning Tree of a weighted graph using Kruskal's algorithm.
 * The edge weights are taken from the edge labels, so each weight is only computed once (when the edge was created).
 * 
 * Returns an Option containing the MST graph if the input graph is connected.
 * Returns nullopt if the graph is disconnected.
 * @tparam G weighted graph type
 * @param graph input graph (by value), reused to store the MST
 * @return Option<G> MST graph (edges keep their weights), or nullopt if graph is disconnected
 */
template<IWeightedGraph G>
fishnet::Option<G> kruskal(G graph) {
    using N = typename G::node_type;
    using E = typename G::edge_type;
    using H = typename G::adj_container_type::hash_function;
    using Eq = typename G::adj_container_type::equality_predicate;

    size_t nodeCount = std::ranges::distance(graph.getNodes());

    if (nodeCount <= 1) {
        return graph;
    }

    // node -> index mapping
    std::unordered_map<N, size_t, H, Eq> nodeToIndex;
    nodeToIndex.reserve(nodeCount);
    size_t idx = 0;
    for (const auto& node : graph.getNodes()) {
        nodeToIndex[node] = idx++;
    }

    std::vector<E> edges = fishnet::util::toVector(graph.getEdges());
    auto weightComparator = [](const E& lhs, const E& rhs) {
        return lhs.getLabel() < rhs.getLabel();
    };
    std::ranges::sort(edges, weightComparator);

    __impl::UnionFind uf(nodeCount);
    size_t edgesAdded = 0;
    size_t targetEdges = nodeCount - 1;
    graph.clear(); // reuse the input graph to store the MST
    for (const auto& edge : edges) {
        size_t fromIdx = nodeToIndex.at(edge.getFrom());
        size_t toIdx = nodeToIndex.at(edge.getTo());
        if (uf.unite(fromIdx, toIdx)) {
            graph.addEdge(edge); // add the weighted edge, so its weight is not recomputed
            edgesAdded++;
            if (edgesAdded == targetEdges) {
                break;
            }
        }
    }
    // Check if graph is connected (MST has exactly n-1 edges)
    if (edgesAdded != targetEdges) {
        return std::nullopt;
    }
    return graph;
}

/**
 * @brief Compute the Minimum Spanning Tree of a graph using Kruskal's algorithm.
 * The graph is wrapped in a weighted graph adapter, which caches the weight of each edge, 
 * so the weight function is called exactly once per edge.
 * 
 * Returns an Option containing the MST graph if the input graph is connected.
 * Returns nullopt if the graph is disconnected.
 * @tparam G graph type
 * @tparam WFn weight function type
 * @param graph input graph (by value)
 * @param weightFunction weight function computing the weight of an edge from its two endpoints
 * @return Option<G> MST graph, or nullopt if graph is disconnected
 */
template<Graph G, typename WFn>
    requires fishnet::graph::Weight<std::invoke_result_t<WFn, const typename G::node_type&, const typename G::node_type&>>
fishnet::Option<G> kruskal(
    G graph,
    WFn weightFunction
) {
    return kruskal(WeightedGraph(std::move(graph), std::move(weightFunction)))
        .transform([](auto && weighted){ return std::move(weighted).unwrap(); });
}

} // namespace fishnet::graph::MST