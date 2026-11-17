#pragma once
#include <fishnet/GraphDecorator.hpp>
#include <fishnet/BFSAlgorithm.hpp>
#include <fishnet/CollectionConcepts.hpp>

namespace fishnet::graph{

template<class G> requires (G::edge_type::isDirected()==true)
class DirectedAcyclicGraph: public GraphDecorator<DirectedAcyclicGraph<G>,G>{
private:
    using Base = GraphDecorator<DirectedAcyclicGraph<G>,G>;
    using N = typename G::node_type;
    using E = typename G::edge_type;

    /**
     * @brief Helper function to test if the insertion of an edge would form a cycle in the graph
     * @param from 
     * @param to 
     * @return true insertion of edge would create cycle
     * @return false graph would still be a DAG
     */
    bool hasCycleAfterAddingEdge(const N & from, const N & to) noexcept{
        return BFS::findPath(this->g,to,from).get().has_value();
    }

public:
    DirectedAcyclicGraph(G && underlyingGraph):Base(std::move(underlyingGraph)){}

    DirectedAcyclicGraph():Base(){}

    bool addEdge(const N & from, const N & to) noexcept{
        if(hasCycleAfterAddingEdge(from,to))
            return false;
        this->g.addEdge(from,to);
        return true;
    }

    bool addEdge(N && from, N && to) noexcept{
        if(hasCycleAfterAddingEdge(from,to))
            return false;
        this->g.addEdge(std::move(from),std::move(to));
        return true;
    }

    bool addEdge(const E & edge) noexcept{
        return addEdge(edge.getFrom(),edge.getTo());
    }

    void addEdges(util::forward_range_of<std::pair<N,N>> auto && edges) noexcept{
        std::ranges::for_each(edges,[this](auto && pair){
            auto && [from,to] = pair;
            this->addEdge(from,to);
        });
    }

    void addEdges(util::forward_range_of<E> auto  && edges) noexcept{
        addEdges(std::views::transform(edges,[](auto && edge){return std::make_pair(edge.getFrom(),edge.getTo());}));
    }

    size_t inDegree(const N & node)const noexcept{
        return fishnet::util::size(this->g.getReachableFrom(node));
    }

    size_t outDegree(const N & node)const noexcept{
        return fishnet::util::size(this->g.getNeighbours(node));
    }

    util::forward_range_of<N> auto rootNodes() const noexcept {
        return this->g.getNodes() | std::views::filter([this](const auto & node){return inDegree(node)==0;});
    }
};
}