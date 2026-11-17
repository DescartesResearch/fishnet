#pragma once
#include <fishnet/GraphModel.hpp>
#include <fishnet/GraphDecorator.hpp>
#include <unordered_set>
#include <algorithm>
#include <cassert>

namespace fishnet::graph{

template<class G, typename L, ILabelledEdge<L> LE = LabelledEdge<typename G::edge_type,L>>
class LabelledGraph: public GraphDecorator<LabelledGraph<G,L,LE>,G,LE>{
    private:
        using N = typename G::node_type;
        using E = typename G::edge_type;
        using Base = GraphDecorator<LabelledGraph<G,L,LE>,G,LE>;
        std::unordered_set<LE> edges;
        std::function<L(const N &, const N &)> labelGenerator;
        static const inline auto NodeEqual = typename G::adj_container_type::equality_predicate{};

        const LE & retrieveLabelledEdge(const N & from, const N & to) const {
            auto it =edges.find(makeEdge(from,to));
            assert(it != edges.end());
            return *it;
        }

    public:
        using edge_type = LE;
        using node_type = N;
        using adj_container_type = typename G::adj_container_type;

        LabelledGraph(fishnet::util::BiFunction<N,N,L> auto labelGenerator) requires std::is_default_constructible_v<G>
            :Base(),labelGenerator(std::move(labelGenerator)){};

        LabelledGraph(G && source, fishnet::util::BiFunction<N,N,L> auto labelGenerator)
            :Base(std::move(source)),labelGenerator(std::move(labelGenerator)){
                edges = fishnet::util::toUnorderedSet(
                    Base::getEdges() | std::views::transform([this](const E & e){return this->makeEdge(e.getFrom(),e.getTo());})
                );
            };

        template<Node FromType, Node ToType> requires std::constructible_from<N,FromType> && std::constructible_from<N,ToType>
        bool addEdge(FromType && from, ToType && to){
            return addEdge(makeEdge(std::forward<FromType>(from),std::forward<ToType>(to)));
        }

        template<typename LabelledEdgeType> requires std::same_as<LE,std::remove_cvref_t<LabelledEdgeType>>
        bool addEdge(LabelledEdgeType && edge){
            auto inserted = Base::addEdge(edge.getFrom(),edge.getTo());
            if (not inserted) {
                return false;
            }
            edges.insert(std::forward<LabelledEdgeType>(edge));
            return true;
        }

        void addEdges(util::forward_range_of<std::pair<N,N>> auto && edges){
            for(auto && [from,to]:edges){
                addEdge(std::forward<decltype(from)>(from),std::forward<decltype(to)>(to));
            }
        }

        void addEdges(util::forward_range_of<LE> auto && edges){
            for(auto && edge:edges){
                addEdge(std::forward<decltype(edge)>(edge));
            }
        }

        template<Node FromType, Node ToType> 
            requires std::constructible_from<N,FromType> && std::constructible_from<N,ToType>
        LE makeEdge(FromType && from, ToType && to) const {
            auto label = labelGenerator(from,to);
            return LE(std::forward<FromType>(from),std::forward<ToType>(to),std::move(label));
        }

        template<Node FromType, Node ToType, typename LabelType> 
            requires std::constructible_from<N,FromType> && std::constructible_from<N,ToType> && std::constructible_from<L, LabelType>
        LE makeEdge(FromType && from, ToType && to, LabelType && label) const {
            return LE(std::forward<FromType>(from),std::forward<ToType>(to),std::forward<LabelType>(label));
        }

        bool containsEdge(const LE & edge) const noexcept {
            return Base::containsEdge(edge.getFrom(),edge.getTo());
        }

        using Base::containsEdge;

        void removeNode(const N & node){
            Base::removeNode(node);
            for(auto it = edges.begin(); it != edges.end();){
                if(NodeEqual(it->getFrom(),node) || NodeEqual(it->getTo(),node)){
                    it = edges.erase(it);
                }else{
                    ++it;
                }
            }
        }

        void removeEdge(const N & from, const N & to){
            Base::removeEdge(from,to);
            edges.erase(makeEdge(from,to));
        }

        void removeEdge(const LE & edge){
            Base::removeEdge(edge.getFrom(),edge.getTo());
            edges.erase(edge);
        }

        auto getEdges() const {
            return std::views::all(edges);
        }

        auto getOutboundEdges(const N & node) const {
            return Base::getOutboundEdges(node) | std::views::transform([this](const E & e){return this->retrieveLabelledEdge(e.getFrom(),e.getTo());});
        }

        auto getInboundEdges(const N & node) const {
            return Base::getInboundEdges(node) | std::views::transform([this](const E & e){return this->retrieveLabelledEdge(e.getFrom(),e.getTo());});
        }

        void clear() {
            Base::clear();
            edges.clear();
        }
};
// Deduction guides
template<Graph G, typename LabelFn>
LabelledGraph(G, LabelFn) -> LabelledGraph<std::remove_cvref_t<G>, std::invoke_result_t<LabelFn, const typename G::node_type&, const typename G::node_type&>>;

}