#pragma once
#include <fishnet/GraphModel.hpp>
#include <fishnet/CollectionConcepts.hpp>
namespace fishnet::graph{
/**
 * @brief Abstract Graph Decorator using CRTP
 * 
 * @tparam DecoratorImpl graph decorator implementation type
 * @tparam G graph type
 * @tparam G::edge_type edge type of graph
 */
template<class DecoratorImpl, class G, Edge E = typename G::edge_type>
class GraphDecorator
{                                                          
protected:
    G g;
    using N = G::node_type;
    GraphDecorator() requires std::is_default_constructible_v<G> = default;
    GraphDecorator(G && source):g(std::move(source)){}
    GraphDecorator(util::forward_range_of<N> auto && nodes):g(std::forward<decltype(nodes)>(nodes)){}
    // Explicitly defaulted due to virtual destructor.Protected to prevent slicing.
    GraphDecorator(const GraphDecorator &) = default;
    GraphDecorator(GraphDecorator &&) = default;
    GraphDecorator & operator=(const GraphDecorator &) = default;
    GraphDecorator & operator=(GraphDecorator &&) = default;
public:
    using edge_type = E;
    using adj_container_type = G::adj_container_type;
    using node_type = N;

    bool addNode(N & node){
        return g.addNode(node);
    }

    bool addNode( N && node){
        return g.addNode(node);
    }

    template<typename... Args>
    bool addNode(const N & node, Args... args){
        return g.addNode(node,args...);
    }

    template<typename... Args>
    bool addNode(N && node, Args... args){
        return g.addNode(std::move(node),args...);
    }

    bool addNodes(util::forward_range_of<N> auto & nodes){
        return g.addNodes(nodes);
    }

    bool addNodes(util::forward_range_of<N> auto && nodes){
        return g.addNodes(nodes);
    }

    bool containsNode(const N & node) const noexcept {
        return g.containsNode(node);
    }

    void removeNode(const N & node) {
        g.removeNode(node);
    }

    bool addEdge(const N & from, const N & to){
        return g.addEdge(from,to);
    }

    bool addEdge(N && from, N && to){
        return g.addEdge(std::move(from),std::move(to));
    }

    bool addEdge(const E & edge){
        return g.addEdge(edge);
    }

    void addEdges(util::forward_range_of<std::pair<N,N>> auto && edges){
        g.addEdges(edges);
    }

    void addEdges(util::forward_range_of<E> auto  && edges) {
        g.addEdges(edges);
    }

    bool containsEdge(const N & from, const N & to) const noexcept{
        return g.containsEdge(from,to);
    }

    bool containsEdge(const E & edge) const noexcept {
        return g.containsEdge(edge);
    }

    void removeEdge(const N & from, const N & to){
        g.removeEdge(from,to);
    }

    void removeEdge(const E & edge){
        g.removeEdge(edge);
    }

    inline E makeEdge(const N & from, const N & to) const {
        return g.makeEdge(from,to);
    }

    auto getNodes() const noexcept{
        return g.getNodes();
    }

    auto getEdges() const {
        return g.getEdges();
    }

    auto getNeighbours(const N & node) const noexcept {
        return g.getNeighbours(node);
    }

    auto getReachableFrom(const N & node) const noexcept {
        return g.getReachableFrom(node);
    }

    auto getOutboundEdges(const N & node) const {
        return g.getOutboundEdges(node);
    }

    auto getInboundEdges(const N & node) const {
        return g.getInboundEdges(node);
    }

    const auto & getAdjacencyContainer() const {
        return g.getAdjacencyContainer();
    }

    void clear(){
        g.clear();
    }

    const G & unwrap() const & noexcept {
        return g;
    }

    G unwrap() && noexcept {
        return std::move(g);
    }

    virtual ~GraphDecorator() = default;   
};
}
