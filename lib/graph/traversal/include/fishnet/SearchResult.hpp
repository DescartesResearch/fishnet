#pragma once
#include <unordered_map>
#include "NodeStatus.hpp"
#include <fishnet/FunctionalConcepts.hpp>
namespace fishnet::graph{
/**
 * @brief CRTP Abstraction of Search Results
 * 
 * @tparam ResultImpl implementing search result type
 * @tparam N node type
 * @tparam Hash hasher type on nodes
 * @tparam Equal comparator type on nodes
 */
template<class ResultImpl, typename N = ResultImpl::node_type,util::HashFunction<N> Hash=std::hash<N>, fishnet::util::BiPredicate<N> Equal = std::equal_to<N>>
class SearchResult
{
protected:
    std::unordered_map<N,NodeStatus,Hash,Equal> nodeStatus;
public:
    using node_type = N;

    SearchResult(){
        nodeStatus = std::unordered_map<N,NodeStatus,Hash,Equal>();
    };

    void onOpen(const N & node){
        static_cast<ResultImpl *>(this)->onOpen(node);
    }

    void onClose(const N & node){
        static_cast<ResultImpl *>(this)->onClose(node);
    }

    void onEdge(const N & from, const N & to){
        static_cast<ResultImpl *>(this)->onEdge(from,to);
    }

    bool stop() const {
        return static_cast<const ResultImpl *>(this)->stop();
    }

    NodeStatus getStatusOfNode(const N & node) const {
        if (this->nodeStatus.contains(node))
             return this->nodeStatus.at(node);
        return NodeStatus::UNKNOWN;
    }

    void open(const N & node){
        if(this->isOpen(node)) 
            return;
        if (this->nodeStatus.contains(node)) {
            this->nodeStatus[node] = NodeStatus::OPEN;
        }
        else {
            this->nodeStatus.try_emplace(node,NodeStatus::OPEN);     
        }
        this->onOpen(node);
    }

    void close(const N & node) {
        if(this->isClosed(node)) 
            return;
        if (this->nodeStatus.contains(node)){
            this->nodeStatus[node] = NodeStatus::CLOSED;
        } else {
            this->nodeStatus.try_emplace(node,NodeStatus::CLOSED);
        }
        this->onClose(node);
    }

    bool isOpen(const N & node) const {
        return this->getStatusOfNode(node) == NodeStatus::OPEN;
    }

    bool isClosed(const N & node) const {
        return this->getStatusOfNode(node) == NodeStatus::CLOSED;
    }

    bool isUnknown(const N & node) const {
        return this->getStatusOfNode(node) == NodeStatus::UNKNOWN;
    }
    virtual ~SearchResult() = default;
};
}