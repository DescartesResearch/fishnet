#pragma once
#include <fishnet/GraphModel.hpp>
#include <fishnet/AdjacencyMap.hpp>
#include <fishnet/BFSAlgorithm.hpp>
#include "DirectedAcyclicGraph.hpp"
#include "LabelledGraph.hpp"
#include "BaseGraph.hpp"

namespace fishnet::graph{
    
/**
 * @brief Simple Undirected Graph type
 * 
 * @tparam N node type
 * @tparam Hash hasher on node type
 * @tparam Equal comparator on node type
 * @tparam AdjContainer Adjacency container used for the graph
 */
template<Node N, util::HashFunction<N> Hash = std::hash<N>, util::BiPredicate<N> Equal = std::equal_to<N>, AdjacencyContainer<N> AdjContainer = AdjacencyMap<N,Hash,Equal>>
using UndirectedGraph = graph::__impl::BaseGraph<UndirectedEdge<N,Hash,Equal>,AdjContainer>;

/**
 * @brief Simple Directed Graph type
 * 
 * @tparam N node type
 * @tparam Hash hasher on node type
 * @tparam Equal comparator on node type
 * @tparam AdjContainer Adjacency container used for the graph
 */
template<Node N, util::HashFunction<N> Hash = std::hash<N>, util::BiPredicate<N> Equal = std::equal_to<N>, AdjacencyContainer<N> AdjContainer = AdjacencyMap<N,Hash,Equal>>
using DirectedGraph = graph::__impl::BaseGraph<DirectedEdge<N,Hash,Equal>,AdjContainer>;

/**
 * @brief Weighted Graph Adapter, constraining the label type to fulfill the weight concept
 * @see LabelledGraph
 * @tparam G base graph type
 * @tparam W weight type
 * @tparam WE weighted edge type, defaulting to LabelledEdge of the base graph's edge type
 */
template<Graph G, Weight W, IWeightedEdge<W> WE = LabelledEdge<typename G::edge_type,W>>
using WeightedGraph = LabelledGraph<G,W,WE>;

class GraphFactory{
public:
    template<Node N, util::HashFunction<N> Hash = std::hash<N>,util::BiPredicate<N> Equal = std::equal_to<N>>
    static auto UndirectedGraph(){
        return graph::__impl::BaseGraph<UndirectedEdge<N,Hash,Equal>,AdjacencyMap<N,Hash,Equal>>();
    }

    static auto UndirectedGraph(AdjacencyContainer auto && adjContainer){
        using AdjacencyContainer_t = std::remove_cvref_t<decltype(adjContainer)>;
        using N = typename AdjacencyContainer_t::node_type;
        using Hash = typename AdjacencyContainer_t::hash_function;
        using Equal = typename AdjacencyContainer_t::equality_predicate;
        return graph::__impl::BaseGraph<UndirectedEdge<N,Hash,Equal>,AdjacencyContainer_t>(std::move(adjContainer));
    }


    template<Node N, util::HashFunction<N> Hash = std::hash<N>,util::BiPredicate<N> Equal = std::equal_to<N>>
    static auto DirectedGraph(){
        return graph::__impl::BaseGraph<DirectedEdge<N,Hash,Equal>,AdjacencyMap<N,Hash,Equal>>();
    }

    static auto DirectedGraph(AdjacencyContainer auto && adjContainer){
        using AdjacencyContainer_t = std::remove_cvref_t<decltype(adjContainer)>;
        using N = typename AdjacencyContainer_t::node_type;
        using Hash = typename AdjacencyContainer_t::hash_function;
        using Equal = typename AdjacencyContainer_t::equality_predicate;
        return graph::__impl::BaseGraph<DirectedEdge<N,Hash,Equal>,AdjacencyContainer_t>(std::move(adjContainer));
    }

    template<Node N, util::HashFunction<N> Hash = std::hash<N>,util::BiPredicate<N> Equal = std::equal_to<N>>
    static auto DAG(){
        return DirectedAcyclicGraph(DirectedGraph<N,Hash,Equal>());
    }

    static auto DAG(AdjacencyContainer auto && adjContainer){
        return DirectedAcyclicGraph(DirectedGraph(std::move(adjContainer)));
    }
};
}