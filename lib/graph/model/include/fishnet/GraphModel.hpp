#pragma once
#include <fishnet/AdjacencyContainer.hpp>
#include <vector>
#include <concepts>
#include "Edge.hpp"

namespace fishnet::graph {

/**
 * @brief Interface for graph types.
 * Every graph type has to inherit from AbstractGraph
 * @tparam GraphImpl graph implementation
 * @tparam E edge type
 * @tparam N node type
 */
template<typename GraphImpl, typename E=GraphImpl::edge_type, typename N = E::node_type, typename A = GraphImpl::adj_container_type>
concept Graph = Node<N> && Edge<E> && 
requires(const GraphImpl & g,GraphImpl & g_mut,  N n, std::vector<N> nodes, E e, std::vector<E> edges, std::vector<std::pair<N,N>> nodePairs){
    {g.getAdjacencyContainer()} -> std::same_as<const A &>;
    {g_mut.addNode(n)} -> std::same_as<bool>;
    {g_mut.addNodes(nodes)} -> std::same_as<bool>;
    {g.containsNode(n)} -> std::same_as<bool>;
    {g_mut.removeNode(n)} -> std::same_as<void>;
    {g_mut.addEdge(n,n)} -> std::same_as<bool>;
    {g_mut.addEdge(e)} -> std::same_as<bool>;
    {g_mut.addEdges(edges)} -> std::same_as<void>;
    {g_mut.addEdges(nodePairs)} -> std::same_as<void>;
    {g.containsEdge(n,n)} -> std::same_as<bool>;
    {g.containsEdge(e)} -> std::same_as<bool>;
    {g_mut.removeEdge(n,n)} -> std::same_as<void>;
    {g_mut.removeEdge(e)} -> std::same_as<void>;
    {g.makeEdge(n,n)} -> std::same_as<E>;
    {g.getNodes()} -> util::forward_range_of<const N>;
    {g.getEdges()} -> util::forward_range_of<E>;
    {g.getNeighbours(n)} -> util::forward_range_of<const N>;
    {g.getReachableFrom(n)} -> util::forward_range_of<const N>;
    {g.getInboundEdges(n)} -> util::forward_range_of<E>;
    {g.getOutboundEdges(n)} -> util::forward_range_of<E>;
    {g_mut.clear()} -> std::same_as<void>;
    typename GraphImpl::node_type;
    typename GraphImpl::edge_type;
    typename GraphImpl::adj_container_type;
};

template<typename GraphImpl, typename Label>
concept ILabelledGraph = Graph<GraphImpl> && ILabelledEdge<typename GraphImpl::edge_type, Label>;

template<typename GraphImpl, typename Weight>
concept IWeightedGraph = Graph<GraphImpl> && IWeightedEdge<typename GraphImpl::edge_type, Weight>;
}