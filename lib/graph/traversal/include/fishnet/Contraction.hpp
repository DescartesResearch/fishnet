#pragma once
#include "BFSAlgorithm.hpp"
#include <future>
#include <mutex>
#include <numeric>
#include <type_traits>
#include <chrono>
#include <unordered_set>
#include <fishnet/GraphModel.hpp>
#include <fishnet/CollectionConcepts.hpp>

namespace fishnet::graph::__impl {

template<typename N>
using QueueType = fishnet::util::BlockingQueue<std::pair<int,std::vector<N>>>;

template<typename N>
using QueuePtr = std::shared_ptr<QueueType<N>>;

/**
 * @brief Helper runnable, taking connected components and merging the vertices to a single vertex using the contract function
 * 
 * @tparam N source node type
 * @tparam R result node type
 * @param queue shared pointer to blocking queue, storing pairs of component-ids and vectors of nodes 
 * @param reduceFunction function that reduces a range of nodes to a single node
 * @return std::vector<std::pair<int,R>> storing the pairs of component-id and the merged nodes (of type R)
 */
template<typename N,typename R>
static std::vector<std::pair<int,R>> mergeWorker(QueuePtr<N>  queue, util::ReduceFunction<std::vector<N>,R> auto const& reduceFunction){
    std::vector<std::pair<int,R>> mergeResult;
    fishnet::Option<std::pair<int,std::vector<N>>> current = std::nullopt;
    do{
        current = queue->take();
        if(not current) {
            queue->putPoisonPill();
            break;
        }
        const auto & [componentId, nodes] = current.value();
        mergeResult.emplace_back(componentId, reduceFunction(std::move(nodes)));
    }while(current);
    return mergeResult;
}

/**
 * @brief Helper runnable, taking connected components and merging the vertices to a single vertex using the contract function
 * 
 * @tparam N node type
 * @param queue shared pointer to blocking queue, storing pairs of component-ids and vectors of nodes 
 * @param reduceFunction function that reduces a range of nodes to a single node
 * @return std::vector<std::pair<int,N>> storing the pairs of component-id and the merged nodes
 */
template<typename N>
static std::vector<std::pair<int,N>> mergeWorker(QueuePtr<N> queue, util::ReduceFunction<std::vector<N>> auto const& reduceFunction){
    return mergeWorker(queue, reduceFunction,util::Identity());
}

template<typename N,typename E>
struct ContractionResult {
    std::vector<N> nodes;
    std::unordered_set<E> edges;
};

/**
 * @brief Generic contract implementation. Edges between nodes fulfilling the contractBiPredicate get contracted. 
 * The incident nodes get merge by reducing a vector of nodes from the source graph to a single node of the target graph
 * The target graph is cleared before inserting the contraction result
 * 
 * @tparam SourceGraphType source graph type
 * @tparam TargetGraphType target graph type
 * @param source source graph (not changed)
 * @param contractBiPredicate specifies when an edge between two nodes from source graph shall be contracted
 * @param reduceFunction specifies how a vector of nodes from the source graph get combined to a single node of the target graph
 * @param mapper maps nodes from the source graph to the node type of the target graph
 * @param output mutable reference to the target graph
 * @param workers amount of concurrent works
 * @return TargetGraphType resulting graph after contracting edges and merging nodes using the target graph node type
 */
template<Graph SourceGraphType, Graph TargetGraphType>
auto contractImpl(const SourceGraphType & source,
    util::BiPredicate<typename SourceGraphType::node_type> auto const & contractBiPredicate,
    util::ReduceFunction<std::vector<typename SourceGraphType::node_type>,typename TargetGraphType::node_type> auto const & reduceFunction,
    util::UnaryFunction<typename SourceGraphType::node_type,typename TargetGraphType::node_type> auto const & mapper,
    TargetGraphType & output,  
    u_int8_t workers = 1)
{
    using N = SourceGraphType::node_type;
    using R = TargetGraphType::node_type;
    if(workers == 0) 
        workers = 1;
    auto queue = std::make_shared<__impl::QueueType<N>>();
    std::vector<std::future<std::vector<std::pair<int,R>>>> futures;
    futures.reserve(workers);
    for(int i = 0; i < workers; i++){
        futures.emplace_back(std::async(std::launch::async,[queue,&reduceFunction](){return __impl::mergeWorker<N,R>(queue,reduceFunction);}));
    }
    auto componentsMap = BFS::connectedComponents(source,queue,contractBiPredicate).nodeMap();
    std::unordered_map<int,R> result;
    queue->putPoisonPill();
    result.reserve(componentsMap.size());
    for(auto & f: futures){
        for(auto & merged: f.get() ){
            result.try_emplace(merged.first,merged.second);
        }
    }
    std::vector<R> nodes;
    for(const auto & node: source.getNodes()){
        if(util::size(source.getNeighbours(node))==0 && util::size(source.getReachableFrom(node))==0){
            nodes.emplace_back(mapper(node));
        }
    }
    nodes.insert(nodes.end(), std::views::values(result).begin(), std::views::values(result).end());

    std::unordered_set<typename TargetGraphType::edge_type> edges;
    for(const auto & edge: source.getEdges()){
        if(componentsMap.at(edge.getFrom()) != componentsMap.at(edge.getTo())){
            edges.insert(
                output.makeEdge(result.at(componentsMap.at(edge.getFrom())), result.at(componentsMap.at(edge.getTo())))
            );
        }
    }
    return ContractionResult<R,typename TargetGraphType::edge_type>{std::move(nodes), std::move(edges)};
}


template<Graph SourceGraphType, Graph TargetGraphType>
void contract(const SourceGraphType & source,
    util::BiPredicate<typename SourceGraphType::node_type> auto const & contractBiPredicate,
    util::ReduceFunction<std::vector<typename SourceGraphType::node_type>,typename TargetGraphType::node_type> auto const & reduceFunction,
    util::UnaryFunction<typename SourceGraphType::node_type,typename TargetGraphType::node_type> auto const & mapper,
    TargetGraphType & output,  
    u_int8_t workers = 1)
{
    auto [nodes, edges] = contractImpl(source,contractBiPredicate,reduceFunction,mapper,output,workers);
    output.addNodes(nodes);
    output.addEdges(edges);
}


template<Graph SourceGraphType, Graph TargetGraphType>
void contractInPlace(SourceGraphType & source,
    util::BiPredicate<typename SourceGraphType::node_type> auto const & contractBiPredicate,
    util::ReduceFunction<std::vector<typename SourceGraphType::node_type>,typename TargetGraphType::node_type> auto const & reduceFunction,
    util::UnaryFunction<typename SourceGraphType::node_type,typename TargetGraphType::node_type> auto const & mapper,
    TargetGraphType & output,  
    u_int8_t workers = 1)
{
    auto [nodes, edges] = contractImpl(source,contractBiPredicate,reduceFunction,mapper,output,workers);
    source.clear();
    output.addNodes(nodes);
    output.addEdges(edges);
}


}

namespace fishnet::graph{

    /**
 * @brief Performs edge contraction algorithm. Edges between nodes fulfilling the contractBiPredicate get contracted. 
 * The incident nodes get merged by first mapping to the node type of the result graph and combining them using the mergeFunction
 * Source graph is not changed, target graph is cleared before inserting the contraction result.
 * 
 * @tparam SourceGraphType source graph type
 * @tparam TargetGraphType target graph type
 * @param source source graph (not changed)
 * @param contractBiPredicate specifies when an edge between two nodes from source graph shall be contracted
 * @param mergeFunction specifies how nodes of the target graph will be merged
 * @param mapper maps nodes from the source graph to the node type of the target graph
 * @param output mutable reference to the target graph
 * @param workers amount of concurrent works
 * @return TargetGraphType resulting graph after contracting edges and merging nodes using the target graph node type
 */
template<Graph SourceGraphType, Graph TargetGraphType>
void contract(const SourceGraphType & source, util::BiPredicate<typename SourceGraphType::node_type> auto const & contractBiPredicate,util::BiFunction<typename SourceGraphType::node_type,typename SourceGraphType::node_type,typename TargetGraphType::node_type> auto const & mergeFunction,
 util::UnaryFunction<typename SourceGraphType::node_type,typename TargetGraphType::node_type> auto const & mapper, TargetGraphType & output,  u_int8_t workers = 1)
{
    using R = typename TargetGraphType::node_type;
    using N = typename SourceGraphType::node_type;
    auto reduceFunction = [&mapper,&mergeFunction](const std::vector<N> & nodes){
        R merged = mapper(nodes.front());
        for(size_t i = 1; i < nodes.size(); i++){
            merged = mergeFunction(merged,mapper(nodes[i]));
        }
        return merged;
    };
    __impl::contract(source,contractBiPredicate,reduceFunction,mapper,output,workers);
}

/**
 * @brief Performs edge contraction algorithm. Edges between nodes fulfilling the contractBiPredicate get contracted. 
 * The incident nodes get merge by reducing a vector of nodes from the source graph to a single node of the target graph.
 * Source graph is not changed, target graph is cleared before inserting the contraction result.
 * 
 * @tparam SourceGraphType source graph type
 * @tparam TargetGraphType target graph type
 * @param source source graph (not changed)
 * @param contractBiPredicate specifies when an edge between two nodes from source graph shall be contracted
 * @param reduceFunction specifies how a vector of nodes from the source graph get combined to a single node of the target graph
 * @param output mutable reference to the target graph
 * @param workers amount of concurrent works
 * @return TargetGraphType resulting graph after contracting edges and merging nodes using the target graph node type
 */
template<Graph SourceGraphType, Graph TargetGraphType>
void contract( const SourceGraphType & source, util::BiPredicate<typename SourceGraphType::node_type> auto const & contractBiPredicate,
    util::ReduceFunction<std::vector<typename SourceGraphType::node_type>,typename TargetGraphType::node_type> auto const & reduceFunction, TargetGraphType & output,  u_int8_t workers = 1)
{
    using N = SourceGraphType::node_type;
    auto mapper = [&reduceFunction](const N & node){return reduceFunction(std::vector<N>({node}));};
    __impl::contract(source,contractBiPredicate,reduceFunction,mapper,output,workers);
}


/**
 * @brief Performs edge contraction algorithm. Edges between nodes fulfilling the contractBiPredicate get contracted. 
 * The incident nodes get merge by reducing a vector of nodes from the source graph to a single node of the target graph.
 * Source graph is not changed, target graph is cleared before inserting the contraction result.
 * 
 * @tparam SourceGraphType source graph type
 * @tparam TargetGraphType target graph type
 * @param source source graph (gets cleared)
 * @param contractBiPredicate specifies when an edge between two nodes from source graph shall be contracted
 * @param reduceFunction specifies how a vector of nodes from the source graph get combined to a single node of the target graph
 * @param output mutable reference to the target graph
 * @param workers amount of concurrent works
 * @return TargetGraphType resulting graph after contracting edges and merging nodes using the target graph node type
 */
template<Graph SourceGraphType, Graph TargetGraphType>
void contractInPlace( SourceGraphType & source, util::BiPredicate<typename SourceGraphType::node_type> auto const & contractBiPredicate,
    util::ReduceFunction<std::vector<typename SourceGraphType::node_type>,typename TargetGraphType::node_type> auto const & reduceFunction, TargetGraphType & output,  u_int8_t workers = 1)
{
    using N = SourceGraphType::node_type;
    auto mapper = [&reduceFunction](const N & node){return reduceFunction(std::vector<N>({node}));};
    __impl::contractInPlace(source,contractBiPredicate,reduceFunction,mapper,output,workers);
}

/**
 * @brief In place edge contraction implementation. Edges between nodes fulfilling the contractBiPredicate get contracted. 
 * The incident nodes get merged by reducing a vector of nodes from the source graph to a single node of the target graph.
 * Source graph is cleared and filled with the contraction result.
 * 
 * @tparam SourceGraphType source graph type
 * @tparam TargetGraphType target graph type
 * @param graph mutable reference to graph to be contracted
 * @param contractBiPredicate specifies when an edge between two nodes from source graph shall be contracted
 * @param reduceFunction specifies how a vector of nodes from the source graph get combined to a single node of the target graph
 * @param workers amount of concurrent works
 * @return TargetGraphType resulting graph after contracting edges and merging nodes using the target graph node type
 */
template<Graph SourceGraphType>
void contractInPlace( SourceGraphType & graph, util::BiPredicate<typename SourceGraphType::node_type> auto const & contractBiPredicate,
    util::ReduceFunction<std::vector<typename SourceGraphType::node_type>,typename SourceGraphType::node_type> auto const & reduceFunction, u_int8_t workers = 1)
{
    using N = SourceGraphType::node_type;
    auto mapper = [&reduceFunction](const N & node){return reduceFunction(std::vector<N>({node}));};
    __impl::contractInPlace(graph,contractBiPredicate,reduceFunction,mapper,graph,workers);
}

/**
 * @brief Contract the graph depending on the contraction predicate, applying the merge function on the nodes of contracted edges
 * 
 * @tparam G graph type
 * @param source graph
 * @param contractBiPredicate BiPredicate, deciding whether an edge between two nodes should be contracted
 * @param mergeFunction BiOperator, defining how two nodes should be merge into a single node
 * @param workers amount of worker threads
 * @return G contracted graph
 */
template<Graph G> requires std::is_constructible_v<G>
G contract(const G & source, fishnet::util::BiPredicate<typename G::node_type> auto const& contractBiPredicate, fishnet::util::BiOperator<typename G::node_type> auto const& mergeFunction, u_int8_t workers = 1){
    G result;
    contract(source,contractBiPredicate,mergeFunction,util::Identity(),result,workers);
    return result;
}

/**
 * @brief Contract the graph depending on the contraction predicate, applying the reduce function to merge clustered nodes
 * 
 * @tparam G graph type
 * @param source graph
 * @param contractBiPredicate BiPredicate, deciding whether an edge between two nodes should be contracted
 * @param reduceFunction Reduce function, defining how a range of nodes should be reduced to a single node
 * @param workers amount of worker threads
 * @return G contracted graph
 */
template<Graph G> requires std::is_constructible_v<G>
G contract(const G & source, fishnet::util::BiPredicate<typename G::node_type> auto const& contractBiPredicate, fishnet::util::ReduceFunction<std::vector<typename G::node_type>> auto const& reduceFunction, u_int8_t workers = 1){
    G result;
    contract(source,contractBiPredicate,reduceFunction,result,workers);
    return result;
}   
}
