#include <gtest/gtest.h>
#include <fishnet/Graph.hpp>
#include <fishnet/TestUtil.hpp>
#include "MoveOnlyAdjacency.hpp"

using namespace fishnet::graph;
using namespace fishnet::test;
using DAGGraphType = decltype(GraphFactory::DAG<size_t>());

class DAGTest: public ::testing::Test{
protected:
    void SetUp() override {
        dag = GraphFactory::DAG<size_t>();
        empty = GraphFactory::DAG<size_t>();
        dag.addEdge(1,2);
        dag.addEdge(2,3);
        dag.addEdge(2,4);
        dag.addEdge(3,4);
        dag.addNode(0);
    }

    DAGGraphType dag;
    DAGGraphType empty;

};

TEST_F(DAGTest, init) {
    auto undirected = GraphFactory::UndirectedGraph<size_t>();
    auto directed = GraphFactory::DirectedGraph<size_t>();
    //auto mustNotCompile = DAGGraphType(std::move(undirected));
    EXPECT_NO_FATAL_FAILURE(DAGGraphType(std::move(directed)));
    auto dagFromAdjContainer = GraphFactory::DAG(AdjacencyMap<size_t>());
}

TEST_F(DAGTest, addNode){
    EXPECT_EMPTY(empty.getNodes());
    EXPECT_TRUE(empty.addNode(1));
    EXPECT_TRUE(empty.containsNode(1));
    EXPECT_SIZE(empty.getNodes(),1);
    EXPECT_FALSE(dag.addNode(1));
}

TEST_F(DAGTest, addEdge){
    EXPECT_TRUE(empty.addEdge(1,2));
    EXPECT_TRUE(empty.containsEdge(1,2));
    EXPECT_TRUE(empty.addEdge(2,3));
    size_t from = 4;
    size_t to = 5;
    EXPECT_TRUE(empty.addEdge(from,to));
    EXPECT_SIZE(empty.getEdges(),3);
    EXPECT_FALSE(dag.addEdge(4,1)); // would create a circle
    EXPECT_TRUE(dag.addEdge(4,5)); // no circle
    EXPECT_SIZE(dag.getEdges(),5);
    using edgeType = DAGGraphType::edge_type;
    edgeType edge {100,101};
    EXPECT_TRUE(empty.addEdge(edge));
}

TEST_F(DAGTest, addEdges){
    std::vector<std::pair<size_t,size_t>> pairs {
        {0,1},{1,2},{2,3},{3,1}/*last edge should not be added*/
    };
    EXPECT_NO_FATAL_FAILURE(empty.addEdges(pairs));
    EXPECT_SIZE(empty.getEdges(),3);
    std::vector<DAGGraphType::edge_type> edges {
        DAGGraphType::edge_type(10,11),DAGGraphType::edge_type(11,10)
    };
    EXPECT_NO_FATAL_FAILURE(empty.addEdges(edges));
    EXPECT_SIZE(empty.getEdges(),4);
}

TEST_F(DAGTest, inDegree){
    EXPECT_EQ(dag.inDegree(1),0);
    EXPECT_EQ(dag.inDegree(2),1);
    EXPECT_EQ(dag.inDegree(4),2);
    EXPECT_EQ(dag.inDegree(100000),0);
    EXPECT_EQ(dag.inDegree(0),0);
    empty.addNode(0);
    EXPECT_EQ(empty.inDegree(0),0);
    empty.addEdge(1,0);
    EXPECT_EQ(empty.inDegree(0),1);
}

TEST_F(DAGTest, outDegree){
    EXPECT_EQ(dag.outDegree(0),0);
    EXPECT_EQ(dag.outDegree(2),2);
    EXPECT_EQ(dag.outDegree(1),1);
    empty.addNode(1);
    EXPECT_EQ(empty.outDegree(1),0);
    empty.addEdge(1,0);
    EXPECT_EQ(empty.outDegree(1),1);
}

TEST_F(DAGTest, rootNodes){
    auto roots = dag.rootNodes();
    std::vector<size_t> expected {0,1};
    EXPECT_SIZE(roots,2);
    EXPECT_CONTAINS(roots,1);
    EXPECT_CONTAINS(roots,0);
}

TEST_F(DAGTest, moveOnlyAdjacency){
    using MoveOnlyDAG = decltype(GraphFactory::DAG(MoveOnlyAdjacency<size_t>()));
    static_assert(!std::is_copy_constructible_v<MoveOnlyDAG> && !std::is_copy_assignable_v<MoveOnlyDAG>);
    static_assert(std::is_move_constructible_v<MoveOnlyDAG> && std::is_move_assignable_v<MoveOnlyDAG>);
    auto source = GraphFactory::DAG(MoveOnlyAdjacency<size_t>());
    source.addEdge(1,2);
    MoveOnlyDAG moved {std::move(source)};
    EXPECT_TRUE(moved.containsEdge(1,2));
    auto assigned = GraphFactory::DAG(MoveOnlyAdjacency<size_t>());
    assigned = std::move(moved);
    EXPECT_TRUE(assigned.containsEdge(1,2));
    EXPECT_FALSE(assigned.addEdge(2,1));
}
