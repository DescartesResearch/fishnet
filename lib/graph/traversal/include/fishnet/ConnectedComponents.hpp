#pragma once
#include "SearchResult.hpp"
#include <unordered_map>
#include <fishnet/BlockingQueue.hpp>
#include <fishnet/GraphModel.hpp>

namespace fishnet::graph{

namespace __impl{
/**
 * @brief Search Result for Connected Components.
 * @tparam N node type
 * @tparam Hash hasher type on nodes
 * @tparam Equal comparator type on nodes
 */
template<typename N,util::HashFunction<N> Hash=std::hash<N>, fishnet::util::BiPredicate<N> Equal = std::equal_to<N>>
class AbstractConnectedComponents: public SearchResult<AbstractConnectedComponents<N,Hash,Equal>,N,Hash,Equal> {
private:
    size_t index;
    int openClosedCounter;
protected:
    virtual void onComponentClose(size_t component_index) noexcept = 0;

    virtual void onComponentOpen(size_t component_index) noexcept = 0;

    virtual void onNodeOpen(const N & node, size_t component_index) noexcept = 0;

    virtual void onEdgeOpen(const N & from, const N & to, size_t component_index) noexcept = 0;
public:
    AbstractConnectedComponents(): SearchResult<AbstractConnectedComponents<N,Hash,Equal>,N,Hash,Equal>(){
        this->index = 0;
        this->openClosedCounter = 0;
    }

    void onOpen(const N & node) noexcept {
        // if openClosedCounter == 0, the BFS search from single vertex is finished, revealing the complete connected component
        if(this->openClosedCounter == 0) { 
            this->onComponentOpen(this->index);
        }
        this->openClosedCounter++;
        onNodeOpen(node, this->index);
    }

    void onClose(const N & node) noexcept {
        this->openClosedCounter--;
        // if openClosedCounter <= 0, the BFS search from single vertex is finished, revealing the complete connected component
        if(this->openClosedCounter <= 0) {
            onComponentClose(this->index); // utilized by concurrent implementation, to fill the queue with components incrementally
            this->index++;
        }
    }

    void onEdge(const N & from, const N & to) noexcept {
        onEdgeOpen(from,to,this->index);
    }

    bool stop() const noexcept{
        return false;
    }

    virtual ~AbstractConnectedComponents() = default;
};
} // namespace __impl

/**
 * @brief Search Result for Connected Components.
 * @tparam N node type
 * @tparam Hash hasher type on nodes
 * @tparam Equal comparator type on nodes
 */
template<typename N,util::HashFunction<N> Hash=std::hash<N>, fishnet::util::BiPredicate<N> Equal = std::equal_to<N>>
class ConnectedComponents: public __impl::AbstractConnectedComponents<N,Hash,Equal>
{
protected:
    std::vector<std::vector<N>> components;

    void onComponentOpen([[maybe_unused]] size_t component_index) noexcept override {
        this->components.push_back(std::vector<N>());
    }

    void onComponentClose([[maybe_unused]] size_t component_index) noexcept override {
        return;
    }

    void onNodeOpen(const N & node, size_t component_index) noexcept override {
        this->components[component_index].push_back(node);
    }

    void onEdgeOpen([[maybe_unused]] const N & from, [[maybe_unused]] const N & to, [[maybe_unused]] size_t component_index) noexcept override {
        return;
    }
    
public:
    using node_type = N;
    ConnectedComponents(): __impl::AbstractConnectedComponents<N,Hash,Equal>(){
        this->components = std::vector<std::vector<N>>();
    }

    std::vector<std::vector<N>> get() const noexcept{
        return this->components;
    }

    /**
     * @brief Get connected components as a Map: ID -> std::vector<N>
     * 
     * @return std::unordered_map<size_t,std::vector<N>>
     */
    std::unordered_map<size_t,std::vector<N>> getAsMap() & noexcept {
        std::unordered_map<size_t,std::vector<N>> map;
        size_t component_number = 0;
        for(const auto & component : components) {
            map.try_emplace(component_number,component);
            component_number++;
        }
        return map;
    }
    /**
     * @brief Get connected components as a Map: ID -> std::vector<N>
     * 
     * @return std::unordered_map<size_t,std::vector<N>>
     */
    std::unordered_map<size_t,std::vector<N>> getAsMap() && noexcept {
        std::unordered_map<size_t,std::vector<N>> map;
        size_t component_number = 0;
        for(auto && component : components) {
            map.try_emplace(component_number,std::move(component));
            component_number++;
        }
        return map;
    }

        /**
     * @brief Get connected components as a Map: Node -> component-id
     * 
     * @return std::unordered_map<N,int,Hash,Equal> storing for each node the component id as a value
     */
    std::unordered_map<N,int,Hash,Equal> nodeMap() const noexcept {
        std::unordered_map<N,int,Hash,Equal> map;
        for(size_t component_number=0; component_number < components.size(); component_number++) {
            for(auto & node : components[component_number]) {
                map.try_emplace(node,component_number);
            }
        }
        return map;
    }

    ~ConnectedComponents() = default;
};

/**
 * @brief Concurrent specialization for computing connected components
 * 
 * @tparam N node type
 * @tparam Hash hasher type on nodes
 * @tparam Equal comparator type on nodes
 */
template<typename N, util::HashFunction<N> Hash= std::hash<N>,fishnet::util::BiPredicate<N> Equal = std::equal_to<N>>
class ConcurrentConnectedComponents: public ConnectedComponents<N,Hash,Equal>
{
private:
    using QueuePtr = std::shared_ptr<fishnet::util::BlockingQueue<std::pair<int,std::vector<N>>>>;
    using Base = ConnectedComponents<N,Hash,Equal>;
    QueuePtr queue;
protected:
    void onComponentClose(size_t component_index) noexcept override {
        this->queue->put(std::make_pair(component_index,this->components.back())); // add connected component to blocking queue on close
    }
public:
    ConcurrentConnectedComponents(QueuePtr q):ConnectedComponents<N,Hash,Equal>(),queue(q){};

    ~ConcurrentConnectedComponents()=default;
};

template<Graph G> 
class Subgraphs: public __impl::AbstractConnectedComponents<typename G::node_type,typename G::adj_container_type::hash_function,typename G::adj_container_type::equality_predicate>{
private:
    std::vector<G> subgraphs;
    fishnet::util::Producer_t<G> graphProducer;
    using Base =__impl::AbstractConnectedComponents<typename G::node_type,typename G::adj_container_type::hash_function,typename G::adj_container_type::equality_predicate>;
    using N = typename G::node_type;
protected:
    void onComponentOpen([[maybe_unused]] size_t component_index) noexcept override {
        this->subgraphs.push_back(graphProducer());
    }

    void onComponentClose([[maybe_unused]]  size_t component_index) noexcept override {
        return;
    }

    void onNodeOpen(const N & node, size_t component_index) noexcept override {
        this->subgraphs[component_index].addNode(node);
    }

    void onEdgeOpen(const N & from, const N & to, size_t component_index) noexcept override {
        this->subgraphs[component_index].addEdge(from,to);
    }

public:
    Subgraphs() requires std::is_default_constructible_v<G>:Base(),graphProducer([](){return G();}){}

    Subgraphs(fishnet::util::Producer<G> auto emptyGraphProducer):Base(),graphProducer(std::move(emptyGraphProducer)){}

    std::vector<G> get() const noexcept{
        return this->subgraphs;
    }
};

} // namespace fishnet::graph