#pragma once
#include <ranges>
#include <algorithm>
#include <fishnet/AdjacencyContainer.hpp>
#include <fishnet/GraphModel.hpp>
#include <fishnet/Edge.hpp>
#include <fishnet/CollectionConcepts.hpp>

namespace fishnet::graph::__impl {
/**
 * @brief Default graph implementation
 * 
 * @tparam E edge type (unlabelled: the adjacency container only stores node pairs, so labels would be lost.
 * Use LabelledGraph / WeightedGraph for labelled edges)
 * @tparam AdjContainer adjacency container type
 */
template<Edge E, AdjacencyContainer<typename E::node_type> AdjContainer>
requires (not HasLabel<E>)
class BaseGraph {
public:
    using node_type = typename AdjContainer::node_type;
    using edge_type = E;
    using adj_container_type = AdjContainer;
private:
    AdjContainer adj;
    using N = node_type;
public:


    BaseGraph() requires std::is_default_constructible_v<AdjContainer>: adj(){};

    BaseGraph(AdjContainer && adjContainer): adj(std::move(adjContainer)){}

    BaseGraph(BaseGraph && other):adj(std::move(other.adj)){}

    BaseGraph(const BaseGraph & other) requires std::is_copy_constructible_v<AdjContainer>
        :adj(other.adj){}

    BaseGraph & operator=(BaseGraph && other)noexcept{
        this->adj = std::move(other.adj);
        return *this;
    }

    BaseGraph & operator=(const BaseGraph & other) requires std::is_copy_assignable_v<AdjContainer> {
        this->adj = other.adj;
        return *this;
    }

    BaseGraph(util::forward_range_of<N> auto && nodes){
        addNodes(std::forward<decltype(nodes)>(nodes));
    };

    bool addNode(const N & node){
        return adj.addNode(node);
    }

    bool addNode(N && node){
        return adj.addNode(node);
    }

    template<typename... Args>
    bool addNode(const N & node, Args... args){
        bool hasChanged = addNode(node);
        return hasChanged && addNode(args...);
    }

    template<typename... Args>
    bool addNode(N && node, Args... args){
        bool hasChanged = addNode(std::move(node));
        return hasChanged && addNode(args...);
    }

    bool addNodes(util::forward_range_of<N> auto && nodes){
        return adj.addNodes(std::forward<decltype(nodes)>(nodes));
    }

    bool containsNode(const N & node) const noexcept {
        return adj.contains(node);
    }

    void removeNode(const N & node) {
        adj.removeNode(node);
    }

    bool addEdge(const N & from, const N & to){
        if (not containsEdge(from,to)) {
            adj.addAdjacency(from,to);
            if constexpr(not E::isDirected()){
                adj.addAdjacency(to,from); // add reversed edge as well if graph is undirected
            }
            return true;
        }
        return false;
    }

    bool addEdge(N && from, N && to){
        if (not containsEdge(from,to)){
            adj.addAdjacency(from,to);
            if constexpr(not E::isDirected()){
                adj.addAdjacency(to,from); // add reversed edge as well if graph is undirected
            }
            return true;
        }
        return false;
    }

    void addEdges(util::forward_range_of<std::pair<N,N>> auto && edges){
        if constexpr(not E::isDirected()){
            std::vector<std::pair<N,N>> reversed; // add reversed edge as well if graph is undirected
            std::ranges::transform(edges,std::back_inserter(reversed),[]( auto & pair){return std::pair(pair.second,pair.first);});
            adj.addAdjacencies(std::move(reversed));
        }
        adj.addAdjacencies(edges);
    }

    void addEdges(util::forward_range_of<E> auto && edges) {
        std::vector<std::pair<N,N>> pairs;
        std::ranges::for_each(edges,[&pairs](const auto & e){pairs.emplace_back(e.getFrom(),e.getTo());});
        addEdges(pairs);
    }

    bool addEdge(const E & edge) {
        N from = edge.getFrom();
        N to = edge.getTo();
        return addEdge(from,to);
    }

    bool containsEdge(const N & from, const N & to) const noexcept {
        if constexpr(E::isDirected())
             return adj.hasAdjacency(from,to);
        else 
            return adj.hasAdjacency(from,to) && adj.hasAdjacency(to,from);
    }

    bool containsEdge(const E & edge) const noexcept {
        return containsEdge(edge.getFrom(),edge.getTo());
    }

    void removeEdge(const N & from, const N & to){
        adj.removeAdjacency(from,to);
        if constexpr(not E::isDirected()){
            adj.removeAdjacency(to,from);
        }
    }

    void removeEdge(const E & edge){
        removeEdge(edge.getFrom(),edge.getTo());
    }

    inline E makeEdge(const N & from, const N & to) const {
        return E(from,to);
    }

    auto getNodes() const noexcept {
        return std::views::all(adj.nodes());
    }

    auto getEdges() const {
        std::unordered_set<E> edgeSet;
        for(const auto & [from,to]: adj.getAdjacencyPairs()){
            edgeSet.emplace(this->makeEdge(from,to));
        }
        std::vector<E> edges(edgeSet.begin(),edgeSet.end());
        return edges;
    }

    auto getNeighbours(const N & node) const noexcept {
        return adj.adjacency(node);
    }

    auto getReachableFrom(const N & node) const noexcept {
        if constexpr(E::isDirected()){
            auto isReachableFrom = [&](const N & from) {return containsEdge(from,node);};
            return getNodes() | std::views::filter(isReachableFrom);
        }else {
            return getNeighbours(node);
        }        
    }

    auto getOutboundEdges(const N & node) const {
        std::vector<E> edges;
        if(not containsNode(node)) return edges;
        for(const auto & neighbour : getNeighbours(node)) {
            edges.emplace_back(this->makeEdge(node,neighbour));
        }
        return edges;
    }

    auto getInboundEdges(const N & node) const {
        std::vector<E> edges;
        if(not containsNode(node)) return edges;
        for(const auto & neighbour : getReachableFrom(node)) {
            edges.emplace_back(this->makeEdge(neighbour,node));
        }
        return edges;
    }

    void clear(){
        adj.clear();
    }

    const AdjContainer & getAdjacencyContainer() const {
        return adj;
    }

    virtual ~BaseGraph()=default;
};
}
