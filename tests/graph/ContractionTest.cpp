#include <gtest/gtest.h>
#include <fishnet/Graph.hpp>
#include <fishnet/Contraction.hpp>
#include "XYNode.h"
#include "GraphTestUtil.h"


struct MergePredicate{
    bool operator()(const XYNode & n1, const XYNode & n2)const{
        return n1.distanceTo(n2) <= 1.0;
    }
};

 struct MergeFunction{
    XYNode operator()(const XYNode & n1, const XYNode & n2)const {
        return XYNode((n1.getX()+n2.getX())/2,(n1.getY() + n2.getY())/2);
    }
};

class ContractionTest: public ::testing::Test{
protected:
    fishnet::graph::UndirectedGraph<XYNode> g;
    std::vector<XYNode> nodes;

    void SetUp() override {
        // Make fully connected graph
        nodes = randomNodes(50);
        for(auto &from: nodes){
            for(auto & to: nodes ){
                if(from != to) g.addEdge(from,to);
            }
        }
    }

};

TEST_F(ContractionTest, SingleWorker){
    MergePredicate Predicate;
    auto result = fishnet::graph::contract(g,MergePredicate(),MergeFunction());
    for(auto & e: result.getEdges()){
        EXPECT_FALSE(Predicate(e.getFrom(),e.getTo()));
    }
}
