#include <gtest/gtest.h>
#include <fishnet/Graph.hpp>
#include <fishnet/TestUtil.hpp>
#include <string>
#include "XYNode.h"
#include "MoveOnlyAdjacency.hpp"

using namespace fishnet::graph;
using namespace fishnet::test;

static auto distance = [](const XYNode & from, const XYNode & to){ return from.distanceTo(to); };
static auto arrowLabel = [](const int & from, const int & to){ return std::to_string(from) + "->" + std::to_string(to); };

using WG = WeightedGraph<UndirectedGraph<XYNode>,double>;
using LG = LabelledGraph<DirectedGraph<int>,std::string>;
using WE = typename WG::edge_type;

static_assert(Graph<WG>);
static_assert(Graph<LG>);
static_assert(IWeightedGraph<WG,double>);
static_assert(ILabelledGraph<LG,std::string>);
static_assert(std::same_as<WE,UndirectedWeightedEdge<XYNode,double>>);
static_assert(std::derived_from<WG,GraphDecorator<WG,UndirectedGraph<XYNode>,WE>>);

class LabelledGraphTest: public ::testing::Test{
protected:
    void SetUp() override {
        graph.addNode(n0,n1,n2,n3);
        graph.addEdge(n0,n1);
        graph.addEdge(n1,n2);
        graph.addEdge(n2,n3);
        /**
         *  n2 (0,1) --- n1 (1,1)
         *   |            |
         *   |           n0 (0,0)
         *  n3 (3,0)
         */
        directed.addEdge(0,1);
        directed.addEdge(1,2);
        directed.addEdge(2,0);
        directed.addEdge(3,1);
    }
    WG graph {distance};
    WG empty {distance};
    LG directed {arrowLabel};
    XYNode n0 {0,0};
    XYNode n1 {1,1};
    XYNode n2 {0,1};
    XYNode n3 {3,0};
};

TEST_F(LabelledGraphTest, addNode){
    EXPECT_TRUE(empty.addNode(n0));
    EXPECT_TRUE(empty.addNode(XYNode{1,1}));
    EXPECT_FALSE(empty.addNode(n0));
    EXPECT_CONTAINS(empty.getNodes(),n0);
    EXPECT_CONTAINS(empty.getNodes(),n1);
    EXPECT_SIZE(empty.getNodes(),2);
    EXPECT_EMPTY(empty.getEdges());
}

TEST_F(LabelledGraphTest, addNodeVarArgs){
    EXPECT_NO_FATAL_FAILURE(empty.addNode(n0,n1));
    EXPECT_CONTAINS(empty.getNodes(),n0);
    EXPECT_CONTAINS(empty.getNodes(),n1);
    EXPECT_SIZE(empty.getNodes(),2);
}

TEST_F(LabelledGraphTest, addNodes){
    std::vector<XYNode> nodes = {n0,n1,n2};
    EXPECT_NO_FATAL_FAILURE(empty.addNodes(nodes));
    EXPECT_CONTAINS_ALL(empty.getNodes(),nodes);
    EXPECT_NO_FATAL_FAILURE(empty.addNodes(std::vector<XYNode>{n3}));
    EXPECT_SIZE(empty.getNodes(),4);
}

TEST_F(LabelledGraphTest, containsNode){
    EXPECT_TRUE(graph.containsNode(n0));
    EXPECT_TRUE(graph.containsNode(XYNode{0,0}));
    EXPECT_FALSE(graph.containsNode(XYNode{-1,-1}));
    EXPECT_FALSE(empty.containsNode(n0));
}

TEST_F(LabelledGraphTest, makeEdge){
    auto generated = graph.makeEdge(n0,n1);
    EXPECT_DOUBLE_EQ(generated.getLabel(),n0.distanceTo(n1));
    auto explicitLabel = graph.makeEdge(n0,n1,42.0);
    EXPECT_DOUBLE_EQ(explicitLabel.getLabel(),42.0);
    EXPECT_EQ(directed.makeEdge(0,1).getLabel(),"0->1");
    EXPECT_FALSE(graph.containsEdge(graph.makeEdge(n0,n3))); // making an edge does not insert it
}

TEST_F(LabelledGraphTest, addEdge){
    EXPECT_TRUE(empty.addEdge(n0,n3));
    EXPECT_TRUE(empty.containsNode(n0));
    EXPECT_TRUE(empty.containsNode(n3));
    EXPECT_TRUE(empty.containsEdge(n0,n3));
    EXPECT_SIZE(empty.getEdges(),1);
    EXPECT_DOUBLE_EQ(empty.getEdges().begin()->getLabel(),n0.distanceTo(n3));
    EXPECT_FALSE(empty.addEdge(n0,n3));
    EXPECT_FALSE(empty.addEdge(n3,n0)); // undirected: same edge
    EXPECT_SIZE(empty.getEdges(),1);
}

TEST_F(LabelledGraphTest, addEdgeLabelled){
    auto edge = empty.makeEdge(n0,n1,42.0);
    EXPECT_TRUE(empty.addEdge(edge));
    EXPECT_TRUE(empty.containsEdge(edge));
    EXPECT_TRUE(empty.containsEdge(n0,n1));
    EXPECT_SIZE(empty.getEdges(),1);
    EXPECT_DOUBLE_EQ(empty.getEdges().begin()->getLabel(),42.0);
    // inserting the same edge again with another label is rejected and keeps the original label
    EXPECT_FALSE(empty.addEdge(empty.makeEdge(n0,n1,-1.0)));
    EXPECT_SIZE(empty.getEdges(),1);
    EXPECT_DOUBLE_EQ(empty.getEdges().begin()->getLabel(),42.0);
}

TEST_F(LabelledGraphTest, addEdges){
    std::vector<std::pair<XYNode,XYNode>> pairs = {{n0,n1},{n1,n2}};
    EXPECT_NO_FATAL_FAILURE(empty.addEdges(pairs));
    EXPECT_TRUE(empty.containsEdge(n0,n1));
    EXPECT_TRUE(empty.containsEdge(n1,n2));
    std::vector<WE> labelled = {empty.makeEdge(n2,n3,7.0),empty.makeEdge(n3,n0,8.0)};
    EXPECT_NO_FATAL_FAILURE(empty.addEdges(labelled));
    EXPECT_CONTAINS_ALL(empty.getEdges(),labelled);
    EXPECT_SIZE(empty.getEdges(),4);
    for(const auto & edge: empty.getEdges()){
        if(edge == labelled[0]) EXPECT_DOUBLE_EQ(edge.getLabel(),7.0);
        else if(edge == labelled[1]) EXPECT_DOUBLE_EQ(edge.getLabel(),8.0);
        else EXPECT_DOUBLE_EQ(edge.getLabel(),edge.getFrom().distanceTo(edge.getTo()));
    }
}

TEST_F(LabelledGraphTest, containsEdge){
    EXPECT_TRUE(graph.containsEdge(n0,n1));
    EXPECT_TRUE(graph.containsEdge(n1,n0));
    EXPECT_TRUE(graph.containsEdge(graph.makeEdge(n1,n2)));
    EXPECT_FALSE(graph.containsEdge(n0,n3));
    EXPECT_FALSE(empty.containsEdge(n0,n1));
    EXPECT_TRUE(directed.containsEdge(0,1));
    EXPECT_FALSE(directed.containsEdge(1,0));
}

TEST_F(LabelledGraphTest, removeEdge){
    EXPECT_NO_FATAL_FAILURE(graph.removeEdge(n0,n1));
    EXPECT_FALSE(graph.containsEdge(n0,n1));
    EXPECT_TRUE(graph.containsNode(n0));
    EXPECT_TRUE(graph.containsNode(n1));
    EXPECT_SIZE(graph.getEdges(),2);
    EXPECT_NO_FATAL_FAILURE(graph.removeEdge(graph.makeEdge(n2,n1)));
    EXPECT_FALSE(graph.containsEdge(n1,n2));
    EXPECT_SIZE(graph.getEdges(),1);
    EXPECT_NO_FATAL_FAILURE(empty.removeEdge(n0,n1));
    EXPECT_EMPTY(empty.getEdges());
}

TEST_F(LabelledGraphTest, removeNode){
    EXPECT_NO_FATAL_FAILURE(graph.removeNode(n1));
    EXPECT_FALSE(graph.containsNode(n1));
    EXPECT_FALSE(graph.containsEdge(n0,n1));
    EXPECT_FALSE(graph.containsEdge(n1,n2));
    // incident labelled edges must be removed as well
    EXPECT_NOT_CONTAINS(graph.getEdges(),graph.makeEdge(n0,n1));
    EXPECT_NOT_CONTAINS(graph.getEdges(),graph.makeEdge(n1,n2));
    EXPECT_SIZE(graph.getEdges(),1);
    EXPECT_NO_FATAL_FAILURE(empty.removeNode(n0));
}

TEST_F(LabelledGraphTest, getEdges){
    std::vector<WE> expected = {graph.makeEdge(n0,n1),graph.makeEdge(n1,n2),graph.makeEdge(n2,n3)};
    EXPECT_CONTAINS_ALL(graph.getEdges(),expected);
    EXPECT_SIZE(graph.getEdges(),3);
    for(const auto & edge: graph.getEdges()){
        EXPECT_DOUBLE_EQ(edge.getLabel(),edge.getFrom().distanceTo(edge.getTo()));
    }
    EXPECT_EMPTY(empty.getEdges());
}

TEST_F(LabelledGraphTest, getNeighbours){
    std::vector<XYNode> neighboursOfN1 = {n0,n2};
    EXPECT_CONTAINS_ALL(graph.getNeighbours(n1),neighboursOfN1);
    EXPECT_SIZE(graph.getNeighbours(n1),2);
    EXPECT_EMPTY(empty.getNeighbours(n0));
    EXPECT_EMPTY(graph.getNeighbours(XYNode{-1,-1}));
}

TEST_F(LabelledGraphTest, getReachableFrom){
    std::vector<XYNode> reachableFromN2 = {n1,n3};
    EXPECT_CONTAINS_ALL(graph.getReachableFrom(n2),reachableFromN2);
    EXPECT_NOT_CONTAINS(graph.getReachableFrom(n2),n0);
    EXPECT_EMPTY(empty.getReachableFrom(n0));
}

TEST_F(LabelledGraphTest, getOutboundEdgesUndirected){
    // undirected: every incident edge is outbound, regardless of its stored orientation
    std::vector<WE> outboundOfN1 = {graph.makeEdge(n1,n0),graph.makeEdge(n1,n2)};
    EXPECT_CONTAINS_ALL(graph.getOutboundEdges(n1),outboundOfN1);
    EXPECT_SIZE(graph.getOutboundEdges(n1),2);
    EXPECT_NOT_CONTAINS(graph.getOutboundEdges(n1),graph.makeEdge(n2,n3));
    EXPECT_EMPTY(empty.getOutboundEdges(n0));
}

TEST_F(LabelledGraphTest, getInboundEdgesUndirected){
    std::vector<WE> inboundOfN1 = {graph.makeEdge(n0,n1),graph.makeEdge(n2,n1)};
    EXPECT_CONTAINS_ALL(graph.getInboundEdges(n1),inboundOfN1);
    EXPECT_SIZE(graph.getInboundEdges(n1),2);
    EXPECT_EMPTY(empty.getInboundEdges(n0));
}

TEST_F(LabelledGraphTest, getOutboundAndInboundEdgesDirected){
    auto outbound = directed.getOutboundEdges(1);
    EXPECT_SIZE(outbound,1);
    EXPECT_EQ(outbound.front().getTo(),2);
    EXPECT_EQ(outbound.front().getLabel(),"1->2");
    auto inbound = directed.getInboundEdges(1);
    EXPECT_SIZE(inbound,2);
    std::vector<int> sources;
    for(const auto & edge: inbound){
        EXPECT_EQ(edge.getTo(),1);
        EXPECT_EQ(edge.getLabel(),std::to_string(edge.getFrom())+"->1");
        sources.push_back(edge.getFrom());
    }
    EXPECT_CONTAINS(sources,0);
    EXPECT_CONTAINS(sources,3);
}

TEST_F(LabelledGraphTest, constructFromExistingGraph){
    UndirectedGraph<XYNode> source;
    source.addEdge(n0,n1);
    source.addEdge(n2,n3);
    WG fromSource {std::move(source),distance};
    EXPECT_TRUE(fromSource.containsEdge(n0,n1));
    EXPECT_TRUE(fromSource.containsEdge(n2,n3));
    EXPECT_SIZE(fromSource.getEdges(),2);
    for(const auto & edge: fromSource.getEdges()){
        EXPECT_DOUBLE_EQ(edge.getLabel(),edge.getFrom().distanceTo(edge.getTo()));
    }
}

TEST_F(LabelledGraphTest, copy){
    WG copy {graph};
    copy.addEdge(n0,n3);
    EXPECT_TRUE(copy.containsEdge(n0,n3));
    EXPECT_FALSE(graph.containsEdge(n0,n3));
    EXPECT_SIZE(copy.getEdges(),4);
    EXPECT_SIZE(graph.getEdges(),3);
    WG assigned {distance};
    assigned = graph;
    EXPECT_SIZE(assigned.getEdges(),3);
    EXPECT_DOUBLE_EQ(assigned.makeEdge(n0,n3).getLabel(),n0.distanceTo(n3)); // label generator is copied
}

TEST_F(LabelledGraphTest, move){
    WG moved {std::move(graph)};
    EXPECT_SIZE(moved.getEdges(),3);
    EXPECT_TRUE(moved.containsEdge(n0,n1));
    WG assigned {distance};
    assigned = std::move(moved);
    EXPECT_SIZE(assigned.getEdges(),3);
    EXPECT_TRUE(assigned.addEdge(n0,n3));
}

TEST_F(LabelledGraphTest, moveOnlyAdjacency){
    using MoveOnlyGraph = decltype(GraphFactory::UndirectedGraph(MoveOnlyAdjacency<XYNode>()));
    using MoveOnlyWG = WeightedGraph<MoveOnlyGraph,double>;
    static_assert(Graph<MoveOnlyWG>);
    static_assert(!std::is_copy_constructible_v<MoveOnlyWG> && !std::is_copy_assignable_v<MoveOnlyWG>);
    static_assert(std::is_move_constructible_v<MoveOnlyWG> && std::is_move_assignable_v<MoveOnlyWG>);
    MoveOnlyWG source {GraphFactory::UndirectedGraph(MoveOnlyAdjacency<XYNode>()),distance};
    source.addEdge(n0,n1);
    MoveOnlyWG moved {std::move(source)};
    EXPECT_TRUE(moved.containsEdge(n0,n1));
    MoveOnlyWG assigned {GraphFactory::UndirectedGraph(MoveOnlyAdjacency<XYNode>()),distance};
    assigned = std::move(moved);
    EXPECT_TRUE(assigned.containsEdge(n0,n1));
    EXPECT_DOUBLE_EQ(assigned.getEdges().begin()->getLabel(),n0.distanceTo(n1));
}

TEST_F(LabelledGraphTest, clear){
    EXPECT_NO_FATAL_FAILURE(graph.clear());
    EXPECT_EMPTY(graph.getNodes());
    EXPECT_EMPTY(graph.getEdges());
    EXPECT_EMPTY(graph.getNeighbours(n1));
    EXPECT_EMPTY(graph.getOutboundEdges(n1));
    EXPECT_EMPTY(graph.getInboundEdges(n1));
    EXPECT_TRUE(graph.addEdge(n0,n1)); // still usable after clear
    EXPECT_SIZE(graph.getEdges(),1);
}
