#include <gtest/gtest.h>
#include <fishnet/Edge.hpp>
#include <memory>
#include <string>
#include "XYNode.h"
using namespace fishnet::graph;

/**
 * Label that counts how often it was copied / moved, to verify that the constructors forward their arguments
 */
struct CountingLabel{
    static inline int copies = 0;
    static inline int moves = 0;
    int value = 0;
    CountingLabel() = default;
    CountingLabel(int value):value(value){}
    CountingLabel(const CountingLabel & other):value(other.value){ copies++; }
    CountingLabel(CountingLabel && other) noexcept :value(other.value){ moves++; }
    CountingLabel & operator=(const CountingLabel &) = default;
    CountingLabel & operator=(CountingLabel &&) = default;
    static void reset(){ copies = 0; moves = 0; }
};

/**
 * Label without default constructor
 */
struct NoDefaultLabel{
    int value;
    explicit NoDefaultLabel(int value):value(value){}
};

using UE = UndirectedEdge<XYNode>;
using DE = DirectedEdge<XYNode>;
using ULE = UndirectedLabelledEdge<XYNode,std::string>;
using DLE = DirectedLabelledEdge<XYNode,std::string>;
using UWE = UndirectedWeightedEdge<XYNode,double>;
using DWE = DirectedWeightedEdge<XYNode,double>;

static_assert(ILabelledEdge<ULE,std::string> && ILabelledEdge<DLE,std::string>);
static_assert(IWeightedEdge<UWE,double> && IWeightedEdge<DWE,double>);
static_assert(!IWeightedEdge<ULE,std::string>, "a string label is not a weight");
static_assert(std::derived_from<ULE,UE> && std::derived_from<DLE,DE>);

// constructors without label require a default initializable label type
static_assert(!std::constructible_from<LabelledEdge<UE,NoDefaultLabel>,XYNode,XYNode>);
static_assert(!std::constructible_from<LabelledEdge<UE,NoDefaultLabel>,UE>);
// edge type must match exactly
static_assert(!std::constructible_from<ULE,DE,std::string>);

using MoveOnlyLabelEdge = LabelledEdge<UE,std::unique_ptr<int>>;
static_assert(!std::copy_constructible<MoveOnlyLabelEdge> && std::move_constructible<MoveOnlyLabelEdge>);

class LabelledEdgeTest: public ::testing::Test{
protected:
    void SetUp() override {
        CountingLabel::reset();
    }
    XYNode from {0,0};
    XYNode to {1,1};
};

TEST_F(LabelledEdgeTest, constructFromNodesAndLabel){
    auto edge = ULE(from,to,std::string("label"));
    EXPECT_EQ(edge.getFrom(),from);
    EXPECT_EQ(edge.getTo(),to);
    EXPECT_EQ(edge.getLabel(),"label");

    auto weighted = DWE(XYNode{2,2},XYNode{3,3},42); // int converted to double
    EXPECT_EQ(weighted.getFrom(),XYNode(2,2));
    EXPECT_EQ(weighted.getTo(),XYNode(3,3));
    EXPECT_DOUBLE_EQ(weighted.getLabel(),42.0);

    auto fromCString = ULE(from,to,"c-string");
    EXPECT_EQ(fromCString.getLabel(),"c-string");
}

TEST_F(LabelledEdgeTest, constructFromNodesDefaultLabel){
    auto edge = ULE(from,to);
    EXPECT_EQ(edge.getFrom(),from);
    EXPECT_EQ(edge.getTo(),to);
    EXPECT_EQ(edge.getLabel(),std::string());

    auto weighted = UWE(from,to);
    EXPECT_DOUBLE_EQ(weighted.getLabel(),0.0); // value initialized
}

TEST_F(LabelledEdgeTest, constructFromEdgeAndLabel){
    UE base {from,to};
    auto fromLValue = ULE(base,"label");
    EXPECT_EQ(fromLValue.getFrom(),from);
    EXPECT_EQ(fromLValue.getTo(),to);
    EXPECT_EQ(fromLValue.getLabel(),"label");
    EXPECT_EQ(base.getFrom(),from); // lvalue edge is copied, not moved from

    auto fromRValue = DWE(DE(from,to),1.5);
    EXPECT_EQ(fromRValue.getFrom(),from);
    EXPECT_EQ(fromRValue.getTo(),to);
    EXPECT_DOUBLE_EQ(fromRValue.getLabel(),1.5);
}

TEST_F(LabelledEdgeTest, constructFromEdgeDefaultLabel){
    UE base {from,to};
    auto edge = ULE(base);
    EXPECT_EQ(edge.getFrom(),from);
    EXPECT_EQ(edge.getTo(),to);
    EXPECT_EQ(edge.getLabel(),std::string());
    EXPECT_DOUBLE_EQ(DWE(DE(from,to)).getLabel(),0.0);
}

TEST_F(LabelledEdgeTest, constructNoDefaultLabel){
    using E = LabelledEdge<UE,NoDefaultLabel>;
    auto edge = E(from,to,NoDefaultLabel(7));
    EXPECT_EQ(edge.getLabel().value,7);
    auto fromEdge = E(UE(from,to),NoDefaultLabel(8));
    EXPECT_EQ(fromEdge.getLabel().value,8);
}

TEST_F(LabelledEdgeTest, labelIsForwarded){
    using E = LabelledEdge<UE,CountingLabel>;
    CountingLabel label {3};

    E copied {from,to,label};
    EXPECT_EQ(CountingLabel::copies,1);
    EXPECT_EQ(CountingLabel::moves,0);
    EXPECT_EQ(copied.getLabel().value,3);

    CountingLabel::reset();
    E moved {from,to,std::move(label)};
    EXPECT_EQ(CountingLabel::copies,0);
    EXPECT_EQ(CountingLabel::moves,1);

    CountingLabel::reset();
    E constructedInPlace {from,to,5}; // label constructed directly from int
    EXPECT_EQ(CountingLabel::copies,0);
    EXPECT_EQ(CountingLabel::moves,0);
    EXPECT_EQ(constructedInPlace.getLabel().value,5);

    CountingLabel::reset();
    E fromEdge {UE(from,to),CountingLabel(9)};
    EXPECT_EQ(CountingLabel::copies,0);
    EXPECT_EQ(CountingLabel::moves,1);
    EXPECT_EQ(fromEdge.getLabel().value,9);
}

TEST_F(LabelledEdgeTest, moveOnlyLabel){
    MoveOnlyLabelEdge edge {from,to,std::make_unique<int>(42)};
    ASSERT_NE(edge.getLabel(),nullptr);
    EXPECT_EQ(*edge.getLabel(),42);
    MoveOnlyLabelEdge moved {std::move(edge)};
    ASSERT_NE(moved.getLabel(),nullptr);
    EXPECT_EQ(*moved.getLabel(),42);
    EXPECT_EQ(moved.getFrom(),from);
}

TEST_F(LabelledEdgeTest, copyAndAssign){
    auto edge = ULE(from,to,"original");
    auto copy = edge;
    EXPECT_EQ(copy.getLabel(),"original");
    EXPECT_EQ(copy.getFrom(),from);
    auto other = ULE(XYNode{5,5},XYNode{6,6},"other");
    other = edge;
    EXPECT_EQ(other.getLabel(),"original");
    EXPECT_EQ(other.getFrom(),from);
    EXPECT_EQ(other.getTo(),to);
}

TEST_F(LabelledEdgeTest, equalityAndHashIgnoreLabel){
    auto e1 = UWE(from,to,1.0);
    auto e2 = UWE(from,to,2.0);
    auto reversed = UWE(to,from,3.0);
    EXPECT_EQ(e1,e2);
    EXPECT_EQ(e1,reversed); // undirected
    EXPECT_EQ(e1.hash(),e2.hash());
    EXPECT_EQ(e1.hash(),reversed.hash());
    EXPECT_EQ(e1,UE(from,to)); // compares equal to the unlabelled edge

    auto d1 = DWE(from,to,1.0);
    auto d2 = DWE(from,to,2.0);
    auto dReversed = DWE(to,from,1.0);
    EXPECT_EQ(d1,d2);
    EXPECT_NE(d1,dReversed); // directed
    EXPECT_EQ(d1.hash(),d2.hash());
}

TEST_F(LabelledEdgeTest, toString){
    auto edge = ULE(from,to,"label");
    EXPECT_EQ(edge.toString(),UE(from,to).toString() + " [label]");
    auto weighted = DWE(from,to,1.5);
    EXPECT_EQ(weighted.toString(),DE(from,to).toString() + " [1.5]");
}
