#pragma once
#include <vector>
#include <algorithm>
#include <ranges>
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

/**
 * @brief Internal Kruskal's implementation
 * 
 * @tparam G graph type
 * @tparam W weight function type
 * @param graph input graph
 * @param weightFunction function computing edge weight from two nodes
 * @param mst output graph to store the MST result
 * @return fishnet::Option<G> reference to mst if connected, nullopt if disconnected
 */
template<Graph G, typename W>
requires std::totally_ordered<std::invoke_result_t<W, const typename G::node_type&, const typename G::node_type&>>
static fishnet::Option<G> kruskalImpl(
    const G& graph,
    W const& weightFunction,
    G && mst
) {
    using N = typename G::node_type;
    using E = typename G::edge_type;
    using H = typename G::adj_container_type::hash_function;
    using Eq = typename G::adj_container_type::equality_predicate;

    auto nodes = graph.getNodes();
    size_t nodeCount = std::ranges::distance(nodes);

    if (nodeCount == 0) {
        return std::move(mst);
    }

    if (nodeCount == 1) {
        for (const auto& node : nodes) {
            mst.addNode(node);
        }
        return std::move(mst);
    }

    // node -> index mapping
    std::unordered_map<N, size_t, H, Eq> nodeToIndex;
    nodeToIndex.reserve(nodeCount);
    size_t idx = 0;
    for (const auto& node : nodes) {
        nodeToIndex[node] = idx++;
    }

    std::vector<E> edges = fishnet::util::toVector(graph.getEdges());
    auto weightComparator = [&weightFunction](const E& lhs, const E& rhs) {
        return weightFunction(lhs.getFrom(), lhs.getTo()) < weightFunction(rhs.getFrom(), rhs.getTo());
    };
    std::ranges::sort(edges, weightComparator);

    UnionFind uf(nodeCount);
    size_t edgesAdded = 0;
    size_t targetEdges = nodeCount - 1;

    for (const auto& edge : edges) {
        size_t fromIdx = nodeToIndex.at(edge.getFrom());
        size_t toIdx = nodeToIndex.at(edge.getTo());
        if (not uf.connected(fromIdx, toIdx)) {
            uf.unite(fromIdx, toIdx);
            mst.addEdge(edge.getFrom(), edge.getTo());
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
    return std::move(mst);
}

} // namespace __impl

/**
 * @brief Compute the Minimum Spanning Tree of a graph using Kruskal's algorithm.
 * 
 * Returns an Option containing the MST graph if the input graph is connected.
 * Returns nullopt if the graph is disconnected.
 * 
 * @tparam G graph type
 * @tparam W weight function type
 * @param graph input graph
 * @param weightFunction BiFunction computing the weight of an edge from its two endpoints
 * @return fishnet::Option<G> MST graph, or nullopt if graph is disconnected
 */
template<Graph G, typename W>
requires std::totally_ordered<std::invoke_result_t<W, const typename G::node_type&, const typename G::node_type&>> && std::is_default_constructible_v<G>
fishnet::Option<G> kruskal(
    const G& graph,
    W const& weightFunction
) {
    return __impl::kruskalImpl<G, W>(graph, weightFunction, G());
}

/**
 * @brief Compute the Minimum Spanning Tree of a graph using Kruskal's algorithm.
 * 
 * Returns an Option containing the MST graph if the input graph is connected.
 * Returns nullopt if the graph is disconnected.
 * 
 * @tparam G graph type
 * @tparam W weight function type
 * @param graph input graph
 * @param weightFunction BiFunction computing the weight of an edge from its two endpoints
 * @param outputGraph graph to store the MST result
 * @return fishnet::Option<G> MST graph, or nullopt if graph is disconnected
 */
template<Graph G, typename W>
requires std::totally_ordered<std::invoke_result_t<W, const typename G::node_type&, const typename G::node_type&>>
fishnet::Option<G> kruskal(
    const G& graph,
    W const& weightFunction,
    G && outputGraph
) {
    return __impl::kruskalImpl<G, W>(graph, weightFunction, std::forward<G>(outputGraph));
}

} // namespace fishnet::graph::MST
