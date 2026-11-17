#include <gtest/gtest.h>
#include <fishnet/Edge.hpp>
#include "IDNode.h"
#include "XYNode.h"
using namespace fishnet::graph;

struct XYNodeOnlyXHash{
    static inline std::hash<double> hasher{};
    size_t operator()(const XYNode & n)const{
        return hasher(n.getX());
    }
};

struct XYNodeOnlyXEqual{
    bool operator()(const XYNode & left, const XYNode & right)const{
        return left.getX() == right.getX();
    }
};

struct XYNodeWeightFunction{
    double operator()(const XYNode & n1, const XYNode & n2) const {
        return n1.distanceTo(n2);
    }
};


TEST(EdgeTest, Init){
    EXPECT_NO_FATAL_FAILURE(UndirectedEdge<IDNode>(IDNode(),IDNode()));
    EXPECT_NO_FATAL_FAILURE(UndirectedEdge<IDNode>(UndirectedEdge<IDNode>(IDNode(),IDNode())));
    EXPECT_NO_FATAL_FAILURE(DirectedEdge<IDNode>(IDNode(),IDNode()));
    EXPECT_NO_FATAL_FAILURE(DirectedEdge<IDNode>(DirectedEdge<IDNode>(IDNode(),IDNode())));
}

TEST(EdgeTest, getFrom){
    auto from = IDNode();
    auto edge = UndirectedEdge<IDNode>(from,IDNode());
    EXPECT_EQ(edge.getFrom(),from);
    EXPECT_NE(edge.getTo(),from);
}

TEST(EdgeTest, getTo){
    auto to = IDNode();
    auto edge = UndirectedEdge<IDNode>(IDNode(),to);
    EXPECT_EQ(edge.getTo(),to);
    EXPECT_NE(edge.getFrom(),to);
}

TEST(EdgeTest, equalAndHash){
    auto u1 = UndirectedEdge<XYNode>({1,0},{0,0});
    auto u2 = UndirectedEdge<XYNode>({0,0},{1,0});
    auto u3 = UndirectedEdge<XYNode>({1,0},{2,2});
    EXPECT_EQ(u1,u2);
    EXPECT_NE(u1,u3);
    EXPECT_NE(u2,u3);
    EXPECT_EQ(u1.hash(),u2.hash());
    EXPECT_NE(u1.hash(), u3.hash());

    auto d1 = DirectedEdge<XYNode>({1,0},{0,0});
    auto d2 = DirectedEdge<XYNode>({0,0},{1,0});
    auto d3 = DirectedEdge<XYNode>({1,0},{0,0});
    EXPECT_NE(d1,d2);
    EXPECT_EQ(d1,d3);
    EXPECT_NE(d2,d3);
    EXPECT_EQ(d1.hash(),d3.hash());
    EXPECT_NE(d1.hash(),d2.hash());
}

TEST(EdgeTest, isDirected){
    EXPECT_TRUE(DirectedEdge<XYNode>::isDirected());
    EXPECT_FALSE(UndirectedEdge<XYNode>::isDirected());
    auto directed = DirectedWeightedEdge<XYNode,double>::isDirected();
    auto undirected = UndirectedWeightedEdge<XYNode,double>::isDirected();
    EXPECT_TRUE(directed);
    EXPECT_FALSE(undirected);
}

TEST(EdgeTest, specificHashAndEqual){
    using E = UndirectedEdge<XYNode,XYNodeOnlyXHash,XYNodeOnlyXEqual>;
    auto e1 = E(XYNode{1,0},XYNode{2,2});
    auto e2 = E(XYNode{1,100},XYNode{2,200});
    EXPECT_EQ(e1,e2); //equal since only the x values are compared
    EXPECT_EQ(e1.hash(),e2.hash());
}



