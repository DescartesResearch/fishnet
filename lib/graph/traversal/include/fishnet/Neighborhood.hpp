#pragma once
#include <fishnet/GraphModel.hpp>
#include "SearchResult.hpp"

namespace fishnet::graph {

template<Graph G>
class Neighborhood: public SearchResult<Neighborhood<G>,typename G::node_type, typename G::adj_container_type::hash_function, typename G::adj_container_type::equality_predicate>{
private:
    using N = typename G::node_type;
    using Hash = typename G::adj_container_type::hash_function;
    using Equal = typename G::adj_container_type::equality_predicate;
    using Base = SearchResult<Neighborhood<G>,N,Hash,Equal>;
    std::unordered_map<N, size_t,Hash,Equal> nodeDistances;
    mutable G subgraph;
    bool stopFlag = false;
    const size_t order;

    void init(const N & start) {
        subgraph.addNode(start);
        nodeDistances[start] = 0;
    }

public:
    using node_type = N;  

    Neighborhood(const N & start,size_t order): Base(),subgraph(),order(order){
        init(start);
    }

    Neighborhood(G && subgraph,const N & start,size_t order): Base(),subgraph(std::move(subgraph)),order(order){
        init(start);
    }
    
    void onOpen(const N & node) noexcept {
        return;
    }

    void onClose(const N & node) noexcept {
        return;
    }

    void onEdge(const N & from, const N & to) noexcept {
        nodeDistances.try_emplace(to,nodeDistances.at(from)+1);
        /*
            this ensures that edges between nodes of k order are not included in k-order neighborhood but only in k+1 order neighborhood
            since k + k >= 2k so edge is not added if both nodes are of order k, but if one node is of order k and the other is of order less than k,
            then edge is added, which is correct since both nodes should be in k-order neighborhood in this case
        */
        if(nodeDistances.at(to) + nodeDistances.at(from) < 2*order) {
            subgraph.addEdge(from,to);
        }
        if(nodeDistances.at(from) > order){
            stopFlag = true;
        }
    }

    bool stop() const noexcept{
        return stopFlag;
    }

    auto get() && noexcept {
        return std::move(subgraph);
    }

    auto get() & noexcept {
        return subgraph;
    }
};
} // namespace fishnet::graph