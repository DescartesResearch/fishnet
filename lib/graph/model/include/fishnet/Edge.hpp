#pragma once
#include <sstream>
#include <concepts>
#include <fishnet/Concepts.hpp>

namespace fishnet::graph{

/**
 * @brief Concept for a Node
 * A node must be equality comparable.
 * @tparam N node type
 */
template<typename N>
concept Node = std::equality_comparable<std::remove_cvref_t<N>>;

/**
 * @brief Interface for an Edge
 * 
 * @tparam E edge implementation type
 * @tparam N node type
 */
template<typename E, typename N=E::node_type>
concept Edge = fishnet::util::HasHashMethod<E> && std::equality_comparable<E> && requires(const E & e, const N & cNodeRef){
    {E(cNodeRef,cNodeRef)};
    {E::isDirected()} -> std::convertible_to<bool>;
    {e.getFrom()} -> std::same_as<const N &>;
    {e.getTo()} -> std::same_as<const N &>;
    typename E::node_type;
    typename E::hash_function;
    typename E::equality_predicate;
};

/**
 * @brief Interface for a labelled Edge
 * A labelled edge has a label of type L.
 * @tparam E edge implementation type
 * @tparam L label type
 */
template<typename E, typename L>
concept ILabelledEdge = Edge<E> && 
requires(const E & e,const std::remove_cvref_t<typename E::node_type> & node, const L & label){
    E(node,node,label);
    {e.getLabel()} -> std::convertible_to<L>;
    typename E::label_type;
};

/**
 * @brief Concept for an edge weight
 * An edge weight (currently) must be a number.
 * @note Could be generalized to Semiring or an ordered monoid
 * @tparam T weight type
 */
template<typename T>
concept Weight = fishnet::math::Number<T>;

/**
 * @brief Interface for a weighted Edge
 * A weighted edge is a labelled edge whose label fulfills the weight concept.
 * @tparam E edge implementation type
 * @tparam W weight type
 */
template<typename E, typename W>
concept IWeightedEdge = ILabelledEdge<E,W> && Weight<W>;

} // fishnet::graph

namespace fishnet::graph::__impl{
    /**
     * @brief Default edge implementation
     * 
     * @tparam N node type
     * @tparam Directed bool deciding if edge is directed
     * @tparam Hash hasher on node type
     * @tparam Equal comparator on node type
     */
    template<Node N,bool Directed, fishnet::util::HashFunction<N> Hash = std::hash<N>, fishnet::util::BiPredicate<N> Equal = std::equal_to<N>> 
    class BaseEdge {
        protected:
            N from;
            N to;
            static inline Equal eq = Equal();
            static inline Hash hasher = Hash();
        public:
            using node_type = N;
            using hash_function = Hash;
            using equality_predicate = Equal;
            
            constexpr static bool isDirected(){
                return Directed;
            }

            BaseEdge(const N & from, const N & to):from(from),to(to){}

            BaseEdge( N && from,  N && to):from(std::move(from)),to(std::move(to)){}

            const N & getFrom()const {
                return from;
            }

            const N & getTo() const{
                return to;
            }

            bool operator==( const BaseEdge<N,false,Hash,Equal> & other)const{
                if constexpr(not Directed){
                    return (eq(from, other.getFrom()) and eq(to,other.getTo())) or
                    (eq(from, other.getTo()) and eq(to, other.getFrom()));
                }
                return false; // directed edge != undirected edge
            }

            bool operator==(const BaseEdge<N,true,Hash,Equal> & other) const {
                if constexpr(Directed){
                    return eq(from,other.getFrom()) && eq(to, other.getTo());
                }
                return false; // directed edge != undirected edge
            }

            size_t inline hash()const{
                if constexpr(Directed){
                    size_t hashFrom = hasher(from);
                    size_t hashTo = hasher(to);
                    constexpr size_t shift = sizeof(size_t) *4;
                    return (hashFrom << shift) | hashTo;
                }else{
                    return hasher(from) ^ hasher(to);
                }
            }

            std::string toString() const noexcept requires fishnet::util::HasToString<N>{
                std::ostringstream oss;                    
                oss << "(" << this->from.toString() << ")";
                if constexpr(Directed){
                    oss << "->";
                }else {
                    oss << "-";
                }
                oss << "(" << this->to.toString() << ")";
                return oss.str();
            }
    };

    /**
     * @brief Default labelled edge implementation, inheriting from edge type
     * 
     * @tparam E edge type (cannot be a labelled edge)
     * @tparam L label type
     */
    template<Edge E, typename L> requires (not fishnet::graph::ILabelledEdge<E,L>)
    class BaseLabelledEdge: public E{
        private:
            L label;
            using N = typename E::node_type;
        public:
            using label_type = L;
            using node_type = typename E::node_type;

            template<typename FromType, typename ToType, typename LabelType>
                requires std::constructible_from<N,FromType> && std::constructible_from<N,ToType> && std::constructible_from<L, LabelType>
            BaseLabelledEdge(FromType && from, ToType && to, LabelType && label):E(std::forward<FromType>(from),std::forward<ToType>(to)),label(std::forward<LabelType>(label)){};

            template<typename FromType, typename ToType>
                requires std::constructible_from<N,FromType> && std::constructible_from<N,ToType> && std::default_initializable<L>
            BaseLabelledEdge(FromType && from, ToType && to):E(std::forward<FromType>(from),std::forward<ToType>(to)),label(){};

            template<typename EdgeType, typename LabelType>
                requires std::same_as<E,std::remove_cvref_t<EdgeType>> && std::constructible_from<L,LabelType>
            BaseLabelledEdge(EdgeType && edge, LabelType && label):E(std::forward<EdgeType>(edge)),label(std::forward<LabelType>(label)){}

            template<typename EdgeType>
                requires std::same_as<E,std::remove_cvref_t<EdgeType>> && std::default_initializable<L>
            BaseLabelledEdge(EdgeType && edge):E(std::forward<EdgeType>(edge)),label(){}
            
            const L & getLabel()const{
                return label;
            }

            std::string toString() const noexcept requires fishnet::util::Printable<L> {
                std::ostringstream oss;                    
                oss << E::toString() << " [" << this->label << "]";
                return oss.str();
            }
    };
} // fishnet::graph::__impl

namespace fishnet::graph{

template<Node N, fishnet::util::HashFunction<N> Hash = std::hash<N>, fishnet::util::BiPredicate<N> Equal = std::equal_to<N>>
using UndirectedEdge = __impl::BaseEdge<N,false,Hash,Equal>;

template<Node N, fishnet::util::HashFunction<N> Hash = std::hash<N>, fishnet::util::BiPredicate<N> Equal = std::equal_to<N>>
using DirectedEdge = __impl::BaseEdge<N,true,Hash,Equal>;

template<Edge E, typename L> requires (not ILabelledEdge<E,L>)
using LabelledEdge = __impl::BaseLabelledEdge<E,L>;

template<Node N, typename L, fishnet::util::HashFunction<N> Hash = std::hash<N>, fishnet::util::BiPredicate<N> Equal = std::equal_to<N>>
using UndirectedLabelledEdge = LabelledEdge<UndirectedEdge<N,Hash,Equal>,L>;

template<Node N, typename L, fishnet::util::HashFunction<N> Hash = std::hash<N>, fishnet::util::BiPredicate<N> Equal = std::equal_to<N>>
using DirectedLabelledEdge = LabelledEdge<DirectedEdge<N,Hash,Equal>,L>;

template<Node N, Weight W, fishnet::util::HashFunction<N> Hash = std::hash<N>, fishnet::util::BiPredicate<N> Equal = std::equal_to<N>>
using UndirectedWeightedEdge = LabelledEdge<UndirectedEdge<N,Hash,Equal>,W>;

template<Node N, Weight W, fishnet::util::HashFunction<N> Hash = std::hash<N>, fishnet::util::BiPredicate<N> Equal = std::equal_to<N>>
using DirectedWeightedEdge = LabelledEdge<DirectedEdge<N,Hash,Equal>,W>;

} // fishnet::graph