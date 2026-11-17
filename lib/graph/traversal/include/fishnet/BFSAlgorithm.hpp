#pragma once
#include <vector>
#include <queue>
#include <memory>
#include <fishnet/BlockingQueue.hpp>
#include <fishnet/GraphModel.hpp>
#include <fishnet/FunctionalConcepts.hpp>

#include "ConnectedComponents.hpp"
#include "SearchPath.hpp"
#include "Neighborhood.hpp"

namespace fishnet::graph::__impl{

/**
 * @brief Generic breadth-first search implementation starting from a node
 * 
 * @tparam G graph type
 * @param g graph
 * @param searchResult mutable reference to search result
 * @param start start node
 * @param predicate BiPredicate to require additional criteria for two nodes to be in relation
 */
template<Graph G>
static void bfs(const G  & g, auto & searchResult, const  typename G::node_type  & start,fishnet::util::BiPredicate<typename G::node_type> auto const& predicate) {
    using N = G::node_type;
    std::queue<N> q;
    q.push(start);
    while(not q.empty()and not searchResult.stop()) {
        N current = q.front();
        q.pop();
        searchResult.open(current);
        for(auto const& neighbour : g.getNeighbours(current)){
            searchResult.onEdge(current,neighbour);
            if (searchResult.isUnknown(neighbour) and predicate(current,neighbour) ) {
                searchResult.open(neighbour);
                q.push(neighbour);
            }
        }
        searchResult.close(current);
    }
}

/**
 * @brief Generic breadth-first search implementation starting from a node
 * 
 * @tparam G graph type
 * @param g graph
 * @param searchResult mutable reference to search result
 * @param start start node
 */
template<Graph G>
static void bfs(const G  & g, auto & searchResult, const  typename G::node_type  & start) {
    bfs(g,searchResult,start,fishnet::util::TrueBiPredicate());
}

/**
 * @brief Generic breadth-first search implementation, traversing the entire graph
 * 
 * @tparam G graph type
 * @param g graph
 * @param searchResult mutable reference to search result
 * @param predicate BiPredicate to require additional criteria for two nodes to be in relation
 */
template<Graph G>
static void bfs_all(const Graph auto & g, auto& searchResult,fishnet::util::BiPredicate<typename G::node_type> auto const& predicate){
    auto nodes = g.getNodes();
    for(auto const& n : nodes){
        if (searchResult.stop()){
            return;
        }
        if(searchResult.isUnknown(n)){
            bfs(g, searchResult,n,predicate);
        }
    }
}

template<Graph G>
static void bfs_all(const Graph auto & g, auto& searchResult){
    bfs_all<G>(g,searchResult,fishnet::util::TrueBiPredicate());
}
}

namespace fishnet::graph::BFS {

/**
 * @brief Compute connected components of the graph
 * 
 * @tparam G graph type
 * @param graph graph
 * @param inRelation BiPredicate indicating whether two nodes are in relation
 * @return ConnectedComponents search result
 */
template<Graph G>
auto connectedComponents(const  G  & graph, fishnet::util::BiPredicate<typename G::node_type> auto const& inRelation)  {
    using H = G::adj_container_type::hash_function;
    using E = G::adj_container_type::equality_predicate;
    auto connectedComponents = ConnectedComponents<typename G::node_type, H, E>();
    __impl::bfs_all<G>(graph,connectedComponents,inRelation);
    return connectedComponents;
}

/**
 * @brief Compute connected components of the graph\
 * Uses  DefaultBiPredicate which is always true, when two nodes share an edge
 * @tparam G graph type
 * @param graph graph
 * @return ConnectedComponents search result
 */
template<Graph G>
auto connectedComponents(const G  & graph) {
    return connectedComponents(graph,fishnet::util::TrueBiPredicate());
}

/**
 * @brief Compute connected components of the graph concurrently
 * 
 * @tparam G graph type
 * @param graph graph
 * @param queue shared pointer to blocking queue, storing pairs of component-ids and vectors of nodes
 * @param inRelation BiPredicate indicating whether two nodes are in relation
 * @return ConcurrentConnectedComponents search result 
 */
template<Graph G>
auto connectedComponents(const G & graph,std::shared_ptr<fishnet::util::BlockingQueue<std::pair<int,std::vector<typename G::node_type>>>>  queue, fishnet::util::BiPredicate<typename G::node_type> auto const& inRelation)  {
    using H = G::adj_container_type::hash_function;
    using E = G::adj_container_type::equality_predicate;
    auto concurrentConnectedComponents = ConcurrentConnectedComponents<typename G::node_type,H,E>(queue);
    __impl::bfs_all<G>(graph,concurrentConnectedComponents,inRelation);
    return concurrentConnectedComponents;
}

/**
 * @brief Compute connected components of the graph concurrently
 * 
 * @tparam G graph type
 * @param graph graph
 * @param queue shared pointer to blocking queue, storing pairs of component-ids and vectors of nodes
 * @return ConcurrentConnectedComponents search result  
 */
template<Graph G>
auto connectedComponents(const G & graph, std::shared_ptr<fishnet::util::BlockingQueue<std::pair<int,std::vector<typename G::node_type>>>>  queue)  {
    return connectedComponents(graph,queue,fishnet::util::TrueBiPredicate());
}

/**
 * @brief Find breadth-first search between between start and goal node
 * 
 * @tparam G graph type
 * @param graph graph
 * @param start start node
 * @param goal target node
 * @return util::input_range_of<typename G::edge_type>  vector of edges from start to target
 */
template<Graph G>
auto findPath(const G & graph, const typename G::node_type & start, const typename G::node_type & goal) {
    using H = G::adj_container_type::hash_function;
    using E = G::adj_container_type::equality_predicate;
    auto p = SearchPath<typename G::edge_type,H,E>(start,goal);
     __impl::bfs(graph,p,start);
    return p;
}

/**
 * @brief Find breadth-first search between between start and goal node
 * 
 * @tparam G graph type
 * @param graph graph
 * @param start start node
 * @param goal target node
 * @return util::input_range_of<typename G::edge_type>  vector of edges from start to target
 */
template<Graph G>
util::input_range_of<typename G::edge_type> auto getPath(const G & graph, const typename G::node_type & start, const typename G::node_type & goal) {
    using N = G::node_type;
    std::optional<std::vector<N>> nodesOpt = findPath(graph,start,goal).get();
    if (not nodesOpt){
        return std::vector<typename G::edge_type>();
    }
    std::vector<N> nodes = *nodesOpt;
    std::vector<typename G::edge_type> edges;
    for(size_t i = 0; i < nodes.size()-1; i++) {
        edges.push_back(graph.makeEdge(nodes[i],nodes[i+1]));
    }
    return edges;
}

template<Graph G, Graph O> requires std::same_as<typename G::node_type, typename O::node_type>
G neighborhood(const G & graph, const typename G::node_type & node, size_t order, O && outputGraph){
    outputGraph.addNode(node); // adding start node to output graph, so that it is included in neighborhood of order 0
    auto neighborhood = Neighborhood(std::forward<O>(outputGraph),node,order);
    __impl::bfs(graph,neighborhood,node);
    return std::move(neighborhood).get();
}

template<Graph G> requires std::is_default_constructible_v<G>
G neighborhood(const G & graph, const typename G::node_type & node, size_t order){
    return neighborhood(graph,node,order,G());
}

template<Graph G> requires (!std::is_default_constructible_v<G> && std::is_copy_constructible_v<G>)
G neighborhood(const G & graph, const typename G::node_type & node, size_t order){
    auto copy = graph;
    copy.clear();
    return neighborhood(graph,node,order,copy);
}

/**
 * @brief Find all subgraphs in the given graph
 * 
 * @tparam G 
 * @param graph input graph
 * @param emptyGraphProducer producer function to create an empty graph of type G
 * @return fishnet::util::forward_range_of<G> range of subgraphs of type G
 */
template<Graph G, typename P, typename R = std::invoke_result_t<std::decay_t<P>>> requires std::same_as<typename G::node_type, typename R::node_type>
fishnet::util::forward_range_of<R> auto subgraphs(const G & graph, P && emptyGraphProducer){
    auto subgraphs = Subgraphs<R>(std::move(emptyGraphProducer));
    __impl::bfs_all<G>(graph,subgraphs);
    return subgraphs.get();
}

/**
 * @brief Find all subgraphs in the given graph
 * 
 * @tparam G 
 * @param graph input graph
 * @return fishnet::util::forward_range_of<G> range of subgraphs of type G
 */
template<Graph G> requires std::is_default_constructible_v<G>
fishnet::util::forward_range_of<G> auto subgraphs(const G & graph){
    auto subgraphs = Subgraphs<G>();
    __impl::bfs_all<G>(graph,subgraphs);
    return subgraphs.get();
}



} // namespace fishnet::graph::BFS