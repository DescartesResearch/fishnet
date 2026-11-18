#include <gtest/gtest.h>
#include <fishnet/Line.hpp>
#include <iostream>
using namespace fishnet::geometry;

TEST(LineTest, InitTwoPoints){
    auto l1 = Line(Vec2D{0,0},Vec2D(1,0));
    EXPECT_EQ(l1.p(),Vec2D(0,0));
    EXPECT_EQ(l1.q(),Vec2D(1,0));
    EXPECT_EQ(Line(Vec2D(1,1),Vec2D(1.11,1.11)).q(), Vec2D(1.11,1.11));
    EXPECT_EQ(Line(Vec2D(1.11,1.11),Vec2D(0,0)).q(), Vec2D(0,0));
    EXPECT_ANY_THROW(Line(Vec2D(),Vec2D()));
}

TEST(LineTest, InitSlopeYIntercept){
    auto l = Line(1,0);
    EXPECT_EQ(l.p(), Vec2D(0,0) );
    EXPECT_EQ(l.q() , Vec2D(1,1));
    auto m = Line(3.5,2);
    EXPECT_EQ(m.p(),Vec2D(0,2));
    EXPECT_EQ(m.q(), Vec2D(1,5.5));
    auto n = Line(-3,2.5);
    EXPECT_EQ(n.p(), Vec2D(0,2.5));
    EXPECT_EQ(n.q(), Vec2D(1,-0.5));
}

TEST(LineTest, CoordinateAxis){
    EXPECT_TRUE(Y_AXIS.direction().isOrthogonal(X_AXIS.direction()));
    EXPECT_EQ(Y_AXIS.p().x,0);
    EXPECT_EQ(Y_AXIS.q().x,0);
    EXPECT_EQ(X_AXIS.p().y,0);
    EXPECT_EQ(X_AXIS.q().y,0);
}

TEST(LineTest, toLine){
    auto l = Line(1,0);
    EXPECT_EQ(l.toLine(),l);
}

TEST(LineTest, direction){
    auto line = Line(Vec2D(0.5,0.5),Vec2D(1,1));
    EXPECT_EQ(line.direction(),Vec2D(0.5,0.5));
}

TEST(LineTest, isVertical){
    Line l {Vec2D(1,1),Vec2D(1,100)};
    EXPECT_TRUE(l.isVertical());
    EXPECT_TRUE(Y_AXIS.isVertical());
    EXPECT_FALSE(Line(1,0).isVertical());
    EXPECT_FALSE(X_AXIS.isVertical());
}

TEST(LineTest, slope){
    Line l {Vec2D(0,0),Vec2D(2,1)};
    EXPECT_EQ(l.slope(),0.5);
    EXPECT_EQ(Y_AXIS.slope(),std::numeric_limits<double>::max());
    EXPECT_EQ(Line(Vec2D(1,1),Vec2D(1,2)).slope(), std::numeric_limits<double>::max());
    EXPECT_EQ(Line(2,-1).slope(), 2);
    EXPECT_EQ(X_AXIS.slope(),0);
    EXPECT_EQ(Line<int>::verticalLine(-1).slope(), std::numeric_limits<double>::max());
    EXPECT_EQ(Line<int>::horizontalLine(1).slope(),0);
}  

TEST(LineTest, yIntercept){
    Line l {Vec2D(2,1), Vec2D(-1,0)};
    EXPECT_TRUE(l.yIntercept().has_value());
    EXPECT_DOUBLE_EQ(l.yIntercept().value(),1.0/3.0);
    EXPECT_EQ(Y_AXIS.yIntercept(),std::nullopt);
    EXPECT_EQ(Line(1,-5).yIntercept(),-5);
    EXPECT_EQ(Line<int>::verticalLine(1).yIntercept(),std::nullopt);
    EXPECT_EQ(Line<int>::horizontalLine(4).yIntercept().value(), 4);
}

TEST(LineTest, contains){
    EXPECT_TRUE(X_AXIS.contains(Vec2D()));
    EXPECT_TRUE(X_AXIS.contains(Vec2D(-10,0)));
    EXPECT_TRUE(Y_AXIS.contains(Vec2D()));
    EXPECT_TRUE(Y_AXIS.contains(Vec2D(0,3)));
    EXPECT_FALSE(Y_AXIS.contains(Vec2D(1,0)));
    Line l (0.5,2);
    EXPECT_TRUE(l.contains(Vec2D(1,2.5)));
    EXPECT_FALSE(l.contains(Vec2D(1,2)));
    EXPECT_TRUE(l.contains(l.intersection(X_AXIS).value()));
}

TEST(LineTest, isLeftisRight) {
    EXPECT_TRUE(Y_AXIS.isLeft(Vec2D<double>{-1,0}));
    EXPECT_TRUE(Y_AXIS.isRight(Vec2D<double>{1,0}));
    EXPECT_FALSE(Y_AXIS.isLeft(Vec2D(0,0)));
    EXPECT_TRUE(X_AXIS.isLeft(Vec2D(0,1)));
    EXPECT_TRUE(X_AXIS.isRight(Vec2D(0,-1)));
    EXPECT_TRUE(Line(1,0).isLeft(Vec2D(0,1)));
}

#include <fishnet/Segment.hpp>
TEST(LineTest, isParallel){
    EXPECT_TRUE(X_AXIS.isParallel(Line<double>::horizontalLine(1)));
    EXPECT_TRUE(Line(1,0).isParallel(Line(1,1)));
    EXPECT_FALSE(Line(1,0).isParallel(Line(2,0)));
    EXPECT_TRUE(Y_AXIS.isParallel(Line<double>::verticalLine(0.5)));
    EXPECT_TRUE(Line(1,0).isParallel(Segment(Vec2D(1,0),Vec2D(2,1))));
    EXPECT_FALSE(Line(1,0).isParallel(Segment(Vec2D(1,1),Vec2D(2,1))));
}

TEST(LineTest, intersects){
    EXPECT_TRUE(X_AXIS.intersects(Y_AXIS));
    EXPECT_TRUE(Line(1.11,0).intersects(Line(3,-1)));
    EXPECT_FALSE(Line<int>::horizontalLine(2).intersects(Line<int>::horizontalLine(-3)));
    EXPECT_FALSE(Y_AXIS.intersects(Line<int>::verticalLine(3000)));
    EXPECT_TRUE(Line(1,0).intersects(Segment(Vec2D(0,2),Vec2D(2,2))));
    EXPECT_FALSE(Line(1,0).intersects(Segment(Vec2D(2,0),Vec2D(3,0))));
}

TEST(LineTest, intersection){
    auto l = Line(Vec2D(0,0),Vec2D(2,2));
    auto m = Line(Vec2D(2,0),Vec2D(0,2));
    auto inter = l.intersection(m);
    EXPECT_EQ(*inter,Vec2D(1,1));
    EXPECT_EQ(l.intersection(Segment(Vec2D(-2,0),Vec2D(-1,0))),std::nullopt);
    EXPECT_EQ(l.intersection(Line(Vec2D(-1,0),Vec2D(1,2))),std::nullopt);
}

TEST(LineTest, equality){
    EXPECT_EQ(Line(1,0),Line(1,0));
    EXPECT_EQ(Line(Vec2D(-1,-1),Vec2D(0,0)),Line(Vec2D(0,0),Vec2D(0.5,0.5)));
    EXPECT_EQ(std::hash<Line<int>>{}(Line(Vec2D(-1,-1),Vec2D(0,0))),std::hash<Line<double>>{}(Line(Vec2D(0,0),Vec2D(0.5,0.5))));
    EXPECT_NE(Line(1,0),Line(-1,0));
    EXPECT_NE(Line<int>::verticalLine(1),Y_AXIS);
    EXPECT_EQ(Line<int>::horizontalLine(0),X_AXIS);
    EXPECT_EQ(std::hash<Line<int>>{}(Line<int>::horizontalLine(0)),std::hash<Line<double>>{}(X_AXIS));
    EXPECT_EQ(Line(Vec2D(0,2),Vec2D(1,0)),Line(-2,2));

}

TEST(LineTest, toString){
    Line l {1,2};
    EXPECT_EQ(l.toString(), "Line y = 1.000000 * x + 2.000000");
    EXPECT_EQ(Y_AXIS.toString(), "Vertical Line x = 0.000000");
    std::cout <<"Testing Console Output: "<< l << std::endl; 
}


// ~15 m building edge in lon/lat
TEST(LineTest, ContainsPointOnShortLonLatLine) {
    auto line = Line(Vec2D(39.2218724, -6.8284641), Vec2D(39.2219653, -6.8285636));
    EXPECT_TRUE(line.contains(Vec2D(39.2219008469421, -6.828494567930446)));
    EXPECT_FALSE(line.contains(Vec2D(39.2219008469421 + 1e-9, -6.828494567930446)));
}
