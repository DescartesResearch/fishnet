#include <gtest/gtest.h>
#include <fishnet/Graph.hpp>
#include <fishnet/MSTAlgorithm.hpp>
#include <fishnet/TestUtil.hpp>
#include "XYNode.h"
#include "GraphTestUtil.h"
#include "MoveOnlyAdjacency.hpp"

using namespace fishnet::graph;
using namespace fishnet::util;
using namespace fishnet::test;

struct DistanceFunction {
    double operator()(const XYNode& n1, const XYNode& n2) const {
        return n1.distanceTo(n2);
    }
};

class MSTAlgorithmTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Simple connected graph: triangle with nodes a, b, c
        // a-b: dist 1, b-c: dist 2, a-c: dist 3
        // MST should be a-b + b-c (total weight 3)
        triangleGraph.addNode(a, b, c);
        triangleGraph.addEdge(a, b);
        triangleGraph.addEdge(b, c);
        triangleGraph.addEdge(a, c);

        // Disconnected graph: two separate edges
        disconnectedGraph.addNode(d, e, f, g);
        disconnectedGraph.addEdge(d, e);
        disconnectedGraph.addEdge(f, g);

        // Line graph: a-b-c-d (already a tree)
        lineGraph.addNode(lineA, lineB, lineC, lineD);
        lineGraph.addEdge(lineA, lineB);
        lineGraph.addEdge(lineB, lineC);
        lineGraph.addEdge(lineC, lineD);

        // Single node
        singleNodeGraph.addNode(single);

        // Square with diagonal (complete graph minus one edge)
        // Nodes at (0,0), (1,0), (1,1), (0,1)
        // MST should pick shortest edges
        squareGraph.addNode(sqA, sqB, sqC, sqD);
        squareGraph.addEdge(sqA, sqB);
        squareGraph.addEdge(sqB, sqC);
        squareGraph.addEdge(sqC, sqD);
        squareGraph.addEdge(sqD, sqA);
        squareGraph.addEdge(sqA, sqC);  // diagonal
    }

    // Triangle graph nodes
    XYNode a{0, 0};
    XYNode b{1, 0};
    XYNode c{0, 1};
    fishnet::graph::UndirectedGraph<XYNode> triangleGraph;

    // Disconnected graph nodes
    XYNode d{10, 10};
    XYNode e{11, 10};
    XYNode f{20, 20};
    XYNode g{21, 20};
    fishnet::graph::UndirectedGraph<XYNode> disconnectedGraph;

    // Line graph nodes
    XYNode lineA{0, 0};
    XYNode lineB{1, 0};
    XYNode lineC{2, 0};
    XYNode lineD{3, 0};
    fishnet::graph::UndirectedGraph<XYNode> lineGraph;

    // Single node
    XYNode single{42, 42};
    fishnet::graph::UndirectedGraph<XYNode> singleNodeGraph;

    // Square graph nodes
    XYNode sqA{0, 0};
    XYNode sqB{1, 0};
    XYNode sqC{1, 1};
    XYNode sqD{0, 1};
    fishnet::graph::UndirectedGraph<XYNode> squareGraph;
};

TEST_F(MSTAlgorithmTest, TriangleGraph) {
    auto mstOpt = MST::kruskal(triangleGraph, DistanceFunction());
    auto expected = fishnet::graph::GraphFactory::UndirectedGraph<XYNode>();
    expected.addEdge(a,b);
    expected.addEdge(a,c);
    ASSERT_VALUE(mstOpt);
    EXPECT_GRAPH_EQ(mstOpt.value(), expected);
}

TEST_F(MSTAlgorithmTest, MovedGraph) {
    auto copy = triangleGraph;
    auto mstOpt = MST::kruskal(std::move(copy), DistanceFunction());
    ASSERT_VALUE(mstOpt);
    auto expected = fishnet::graph::GraphFactory::UndirectedGraph<XYNode>();
    expected.addEdge(a,b);
    expected.addEdge(a,c);
    EXPECT_GRAPH_EQ(mstOpt.value(), expected);
}

TEST_F(MSTAlgorithmTest, LValueGraphIsNotModified) {
    auto mstOpt = MST::kruskal(triangleGraph, DistanceFunction());
    ASSERT_VALUE(mstOpt);
    EXPECT_SIZE(triangleGraph.getEdges(), 3);
    EXPECT_TRUE(triangleGraph.containsEdge(b,c));
}

TEST_F(MSTAlgorithmTest, MoveOnlyAdjacency) {
    // a move only graph can only be passed as rvalue, so this only compiles if kruskal never copies the graph
    auto moveOnlyGraph = GraphFactory::UndirectedGraph(MoveOnlyAdjacency<XYNode>());
    static_assert(!std::is_copy_constructible_v<decltype(moveOnlyGraph)>);
    moveOnlyGraph.addEdge(a,b);
    moveOnlyGraph.addEdge(b,c);
    moveOnlyGraph.addEdge(a,c);
    auto mstOpt = MST::kruskal(std::move(moveOnlyGraph), DistanceFunction());
    ASSERT_VALUE(mstOpt);
    auto mst = std::move(mstOpt).value();
    EXPECT_SIZE(mst.getEdges(), 2);
    EXPECT_TRUE(mst.containsEdge(a,b));
    EXPECT_TRUE(mst.containsEdge(a,c));
    EXPECT_FALSE(mst.containsEdge(b,c));
}

TEST_F(MSTAlgorithmTest, DisconnectedGraphReturnsNullopt) {
    auto mstOpt = MST::kruskal(disconnectedGraph, DistanceFunction());
    EXPECT_EMPTY(mstOpt);
}

TEST_F(MSTAlgorithmTest, LineGraphAlreadyTree) {
    auto mstOpt = MST::kruskal(lineGraph, DistanceFunction());
    ASSERT_VALUE(mstOpt);
    EXPECT_GRAPH_EQ(mstOpt.value(), lineGraph);
}

TEST_F(MSTAlgorithmTest, SingleNode) {
    auto mstOpt = MST::kruskal(singleNodeGraph, DistanceFunction());
    ASSERT_VALUE(mstOpt);
    EXPECT_GRAPH_EQ(mstOpt.value(), singleNodeGraph);
}

TEST_F(MSTAlgorithmTest, EmptyGraph) {
    fishnet::graph::UndirectedGraph<XYNode> empty;
    auto mstOpt = MST::kruskal(empty, DistanceFunction());
    ASSERT_VALUE(mstOpt);
    EXPECT_GRAPH_EQ(mstOpt.value(), empty);
}

TEST_F(MSTAlgorithmTest, SquareGraph) {
    auto mstOpt = MST::kruskal(squareGraph, DistanceFunction());
    ASSERT_VALUE(mstOpt);
    auto mst = std::move(mstOpt).value();
    

    // 4 nodes, 3 edges in MST
    EXPECT_SIZE(mst.getNodes(), 4);
    EXPECT_SIZE(mst.getEdges(), 3);

    // Verify all nodes are present
    EXPECT_CONTAINS(mst.getNodes(), sqA);
    EXPECT_CONTAINS(mst.getNodes(), sqB);
    EXPECT_CONTAINS(mst.getNodes(), sqC);
    EXPECT_CONTAINS(mst.getNodes(), sqD);

    // Verify connected: each node should have at least 1 neighbor
    EXPECT_GE(std::ranges::distance(mst.getNeighbours(sqA)), 1);
    EXPECT_GE(std::ranges::distance(mst.getNeighbours(sqB)), 1);
    EXPECT_GE(std::ranges::distance(mst.getNeighbours(sqC)), 1);
    EXPECT_GE(std::ranges::distance(mst.getNeighbours(sqD)), 1);
}
