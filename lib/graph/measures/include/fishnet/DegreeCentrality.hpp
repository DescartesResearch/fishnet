#pragma once
#include <fishnet/Graph.hpp>
#include <vector>
namespace fishnet::graph {
/**
 * @brief Degree Centrality functor
 * Returns a vector of pairs, with each pair storing the node and the degree centrality value
 */
struct DegreeCentrality{
    template<Graph G>
    std::vector<std::pair<typename G::node_type,size_t>> operator() (const G & graph) const{
        std::vector<std::pair<typename G::node_type,size_t>> result;
        auto nodes = graph.getNodes();
        result.reserve(fishnet::util::size(nodes));
        std::transform(nodes.begin(),nodes.end(),std::back_inserter(result),[&graph]( const auto &  node){return std::make_pair(node,fishnet::util::size(graph.getNeighbours(node)));}); 
        return result;
    }
};
}