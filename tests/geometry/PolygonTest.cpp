#include <gtest/gtest.h>
#include <memory>
#include <fishnet/TestUtil.hpp>
#include "ShapeSamples.h"
#include <fishnet/Polygon.hpp>
#include <fishnet/IGeometry.hpp>
#include <list>

using namespace fishnet::geometry;
using namespace fishnet::test;

static_assert(Shape<Polygon<double>>);
static_assert(IPolygon<Polygon<double>>);

class PolygonTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto boundary = LinearRingSamples::COMPLEX_RING;
        polygon = std::make_unique<Polygon<double>>(boundary,holes);
        

    }
    std::unique_ptr<Polygon<double>> polygon;
    Ring<double> h1 = LinearRingSamples::triangle({0,0},{1,1},{1,0});
    Ring<double> h2 = LinearRingSamples::aaBB({0,3},{1,2});
    std::vector<Ring<double>> holes = {h1,h2};
};

TEST_F(PolygonTest, initBasic){
    EXPECT_NO_FATAL_FAILURE(polygon->getBoundary());
    EXPECT_NO_FATAL_FAILURE(polygon->getHoles());
    SimplePolygon<double> boundary = SimplePolygonSamples::aaBB(Vec2D(0,0),Vec2D(4,4));
    std::vector<Ring<double>> twoHoles {
        LinearRingSamples::triangle(Vec2D(1,1),Vec2D(2,2),Vec2D(1,2)),
        LinearRingSamples::aaBB(Vec2D(2.5,3),Vec2D(3,2.5))
    };
    Polygon<double> boxWithTwoHoles = Polygon<double>(boundary,twoHoles);
    EXPECT_NO_FATAL_FAILURE(boxWithTwoHoles.getHoles());
    EXPECT_NO_FATAL_FAILURE(Polygon<double>(LinearRingSamples::aaBB(Vec2D(-1,1),Vec2D(1,-1)))); // no inner ring constructor (ring,{})
    EXPECT_NO_FATAL_FAILURE(Polygon<double>(LinearRingSamples::aaBB(Vec2D(-1,1),Vec2D(1,-1)),std::vector<SimplePolygon<double>>({SimplePolygonSamples::triangle(Vec2D(0,0),Vec2D(0.5,0),Vec2D(0,0.5))}))); // (ring,vec<polygon>)
    EXPECT_NO_FATAL_FAILURE(Polygon<double>(LinearRingSamples::aaBB(Vec2D(-1,1),Vec2D(1,-1)),std::list<SimplePolygon<double>>({SimplePolygonSamples::aaBB(Vec2D(0,0.5),Vec2D(0.5,0))}))); // (ring, list<polygon>)
    EXPECT_NO_FATAL_FAILURE(Polygon<double>(LinearRingSamples::aaBB(Vec2D(-1,1),Vec2D(1,-1)),std::list<Ring<double>>({LinearRingSamples::aaBB(Vec2D(0,0.5),Vec2D(0.5,0))}))); // (ring, list<ring>)
    EXPECT_NO_FATAL_FAILURE(Polygon<double>(SimplePolygonSamples::aaBB(Vec2D(-1,1),Vec2D(1,-1)),std::vector<Ring<double>>({LinearRingSamples::aaBB(Vec2D(0,0.5),Vec2D(0.5,0))}))); // (polygon, vec<ring>)
    EXPECT_NO_FATAL_FAILURE(Polygon<double>(SimplePolygonSamples::aaBB(Vec2D(-1,1),Vec2D(1,-1)),std::list<Ring<double>>({LinearRingSamples::aaBB(Vec2D(0,0.5),Vec2D(0.5,0))}))); // (polygon, vec<ring>)
    EXPECT_NO_FATAL_FAILURE(Polygon<double>(SimplePolygonSamples::aaBB(Vec2D(-1,1),Vec2D(1,-1)),std::vector<SimplePolygon<double>>({SimplePolygonSamples::aaBB(Vec2D(0,0.5),Vec2D(0.5,0))}))); // (polygon, vec<ring>)
}

TEST_F(PolygonTest, initInvalidInput){
    SimplePolygon<double> boundary = SimplePolygonSamples::aaBB(Vec2D(0,0),Vec2D(4,4));
    auto holeOutsideOfBoundary = SimplePolygonSamples::aaBB(Vec2D(-1,-1),Vec2D(0,-2));
    EXPECT_ANY_THROW(Polygon<double>(boundary,{holeOutsideOfBoundary}));
    auto holeCrossingBoundary = LinearRingSamples::triangle(Vec2D(1,1),Vec2D(0.5,0.5),Vec2D(-0.05,1));
    EXPECT_ANY_THROW(Polygon<double>(boundary,{holeCrossingBoundary}));
    auto hole1 = SimplePolygonSamples::aaBB(Vec2D(1,1),Vec2D(2,2));
    auto hole2 = SimplePolygonSamples::triangle(Vec2D(0.5,0.5),Vec2D(1.5,0.5),Vec2D(1.5,1.5));
    EXPECT_ANY_THROW(Polygon<double>(boundary,{hole1,hole2})); // intersecting holes
}

TEST_F(PolygonTest, initEdgeCases) {
    SimplePolygon<double> boundary = SimplePolygonSamples::aaBB(Vec2D(0,0),Vec2D(4,4));
    auto rhombusTouchingBoundary = SimplePolygonSamples::aaRhombus(Vec2D(2,2),2);
    EXPECT_NO_FATAL_FAILURE(Polygon<double>(boundary,{rhombusTouchingBoundary})); 
    auto rhombus1 = SimplePolygonSamples::aaRhombus(Vec2D(1,1),1);
    auto rhombus2 = SimplePolygonSamples::aaRhombus(Vec2D(3,1),1);
    auto triangle = SimplePolygonSamples::triangle(Vec2D(2,1),Vec2D(3,2),Vec2D(1,2));
    EXPECT_NO_FATAL_FAILURE(Polygon<double>(boundary,{rhombus1,rhombus2,triangle})); // touching but not intersecting
}

TEST_F(PolygonTest, getBoundary) {
    EXPECT_EQ(polygon->getBoundary(),LinearRingSamples::COMPLEX_RING);
}

TEST_F(PolygonTest, getHoles) {
    EXPECT_CONTAINS_ALL(polygon->getHoles(),std::vector<Ring<double>> {h1,h2});
    EXPECT_SIZE(polygon->getHoles(),2);
    auto noHolesPolygon = Polygon<int>(Ring<int>({Vec2D(1,1),Vec2D(2,2),Vec2D(5,4)}));
    EXPECT_EMPTY(noHolesPolygon.getHoles());
}

TEST_F(PolygonTest, isSimple) {
    EXPECT_FALSE(polygon->isSimple());
    EXPECT_TRUE(Polygon<double>(SimplePolygonSamples::COMPLEX_BOUNDARY).isSimple());
}

TEST_F(PolygonTest, toSimple) {
    auto simpleFromComplex = polygon->toSimple();
    EXPECT_EMPTY(simpleFromComplex.getHoles());
    EXPECT_SIZE(polygon->getHoles(),2);
}

TEST_F(PolygonTest, area) {
    auto unitSquare = Polygon(SimplePolygonSamples::aaBB(Vec2D(0,0),Vec2D(1,1)));
    EXPECT_EQ(unitSquare.area(),1.0);
    auto unitSquareWithRhombusInside = Polygon(unitSquare.toSimple(),{SimplePolygonSamples::aaRhombus(Vec2D(0.5,0.5),0.5)});
    EXPECT_EQ(unitSquareWithRhombusInside.area(),0.5);
    EXPECT_EQ(polygon->area(),polygon->getBoundary().area() - h1.area()-h2.area());
}

TEST_F(PolygonTest, centroid) {
    auto unitSquareWithHoleInLowerHalf = Polygon<double>(SimplePolygonSamples::aaBB(Vec2D(0,0),Vec2D(1,1)),{SimplePolygonSamples::aaBB(Vec2D(0,0),Vec2D(1,0.5))});
    EXPECT_EQ(unitSquareWithHoleInLowerHalf.centroid(), Vec2D(0.5,0.75));
}

TEST_F(PolygonTest, centroidOfSliver) {
    // building footprint in WGS84 with an area of ~1e-13, below epsilon once multiplied with the centroid
    auto sliver = Polygon<double>(Ring<double>(std::vector<Vec2DReal>{{32.85259276854312, -3.9553591931308945},{32.8525946353636, -3.95535907994086},{32.85259464216783, -3.9553591931308945}}));
    auto centroid = sliver.centroid();
    EXPECT_NEAR(centroid.x, 32.852594, 1e-6);
    EXPECT_NEAR(centroid.y, -3.955359, 1e-6);
}

TEST_F(PolygonTest, containsPoint){
    EXPECT_TRUE(polygon->contains(Vec2D(0,0)));
    EXPECT_TRUE(polygon->contains(Vec2D(-1.5,2)));
    EXPECT_FALSE(polygon->contains(Vec2D(3,0)));
    EXPECT_FALSE(polygon->contains(Vec2D(0.5,2.5)));
    EXPECT_FALSE(polygon->contains(Vec2D(0.8,0.2)));
}

TEST_F(PolygonTest, containsSegment){
    EXPECT_TRUE(polygon->contains(Segment({0,1},{2,2})));
    EXPECT_TRUE(polygon->contains(Segment({0,0},{1,1})));
    EXPECT_FALSE(polygon->contains(Segment({0,0.5},{2,0.5})));
    EXPECT_FALSE(polygon->contains(Segment({3,0},{0,1})));
}

TEST_F(PolygonTest, isInside) {
    EXPECT_TRUE(polygon->isInside(Vec2D{-2,2}));
    EXPECT_TRUE(polygon->isInside(Vec2D{0.5,1}));
    EXPECT_TRUE(polygon->isInside(Vec2D{0,0})); // is on boundary of inner hole
    EXPECT_FALSE(polygon->isInside(Vec2D{-1,1})); // outside
    EXPECT_FALSE(polygon->isInside(Vec2D{0.5,2.5})); // inside of hole
    EXPECT_FALSE(polygon->isInside(Vec2D{0,4})); // is on boundary
}

TEST_F(PolygonTest, isOnBoundary){
    EXPECT_TRUE(polygon->isOnBoundary(Vec2D(2,0)));
    EXPECT_FALSE(polygon->isOnBoundary(Vec2D(0,0)));
}

TEST_F(PolygonTest, isOutside){
    EXPECT_TRUE(polygon->isOutside(Vec2D(0,5)));
    EXPECT_TRUE(polygon->isOutside(Vec2D(0.5,2.5))); //inside of hole
    EXPECT_FALSE(polygon->isOutside(Vec2D(0,-0.5)));
    EXPECT_FALSE(polygon->isOutside(Vec2D(0,0)));
}

TEST_F(PolygonTest, containsInHole) {
    EXPECT_TRUE(polygon->containsInHole(SimplePolygon(h1)));
    EXPECT_FALSE(polygon->containsInHole(SimplePolygon<double>({{0,0},{2,0},{-1,2}})));
}

TEST_F(PolygonTest, intersects) {
    EXPECT_TRUE(polygon->intersects(Line<double>::verticalLine(0)));
    EXPECT_FALSE(polygon->intersects(Line<double>({0,4},{2,3}))); // line only touches
    EXPECT_FALSE(polygon->intersects(Line<double>::horizontalLine(5)));
    EXPECT_FALSE(polygon->intersects(Segment({0.3,2.5},{0.7,2.8}))); // inside of hole
    EXPECT_TRUE(polygon->intersects(Segment({0.5,0.3},{2,1})));
    EXPECT_TRUE(polygon->intersects(Segment({0,-1},{-3,1})));
}

TEST_F(PolygonTest, intersections){
    std::vector<Vec2D<double>> expected = {
        {0,4},{0,3},{0,2},{0,1},{0,0},{0,-0.5}
    };
    EXPECT_UNSORTED_RANGE_EQ(polygon->intersections(Y_AXIS),expected);
    expected = {
        {1,2.5},{2,2.5}
    };
    EXPECT_UNSORTED_RANGE_EQ(polygon->intersections(Segment({0.1,2.5},{4,2.5})),expected);
}

TEST_F(PolygonTest, containsPolygon) {
    EXPECT_TRUE(polygon->contains(SimplePolygonSamples::aaBB({2,1.5},{3,1})));
    Polygon<double> squareWithRhombusHole {
        SimplePolygonSamples::aaBB({1,1},{2,0}),
        {SimplePolygonSamples::aaRhombus({1.5,0.5},0.5)}
    };
    EXPECT_TRUE(polygon->contains(squareWithRhombusHole));
    EXPECT_FALSE(polygon->contains(SimplePolygon<double>(h1)));
    EXPECT_FALSE(polygon->contains(Polygon<double>(LinearRingSamples::aaBB({0,1},{1,0}))));
    EXPECT_FALSE(polygon->contains(SimplePolygonSamples::triangle({0.3,2.2},{0.7,2.8},{0.6,2.1})));
    EXPECT_FALSE(polygon->contains(SimplePolygonSamples::aaRhombus({2.5,0},3)));
    EXPECT_TRUE(polygon->contains(*polygon));
}

TEST_F(PolygonTest, crossesPolygon) {
    EXPECT_FALSE(polygon->crosses(*polygon));
    Polygon<double> polygonWithRhombusHole{
        SimplePolygonSamples::aaBB({-1,5},{1,3.5}),
        {SimplePolygonSamples::aaRhombus({0,4},0.5)}
    };
    EXPECT_TRUE(polygon->crosses(polygonWithRhombusHole));
    EXPECT_TRUE(polygon->getBoundary().crosses(polygonWithRhombusHole.getBoundary()));
    EXPECT_FALSE(polygon->crosses(SimplePolygonSamples::triangle({0.2,2.2},{0.3,2.8},{0.5,2.5}))); //inside hole
    EXPECT_TRUE(polygon->crosses(SimplePolygonSamples::triangle({0.6,0.5},{1.5,1},{1.3,0.2}))); // crosses hole boundary inside
}

TEST_F(PolygonTest, touchesPolygon) {
    EXPECT_FALSE(polygon->touches(*polygon));
    Polygon<double> squareWithSquareHole {
        SimplePolygonSamples::aaBB({-2,-1},{0,-3}),
        {SimplePolygonSamples::aaBB({-1.5,-1.5},{-0.5,-2.5})}
    };
    EXPECT_TRUE(polygon->touches(squareWithSquareHole));
    EXPECT_TRUE(squareWithSquareHole.touches(*polygon));
    EXPECT_FALSE(polygon->touches(SimplePolygonSamples::aaRhombus({0,4},2)));
    EXPECT_TRUE(polygon->touches(SimplePolygonSamples::triangle({0.5,0.2},{0.5,0.3},{1,0.5}))); // touches boundary inside
}

TEST_F(PolygonTest, distance) {
    EXPECT_EQ(polygon->distance(*polygon), 0); // a polygon has no gap to itself
    Polygon<double> touchingDistance {
        SimplePolygonSamples::aaBB({-2,-1},{0,-3}),
        {SimplePolygonSamples::aaBB({-1.5,-1.5},{-0.5,-2.5})}
    };
    EXPECT_EQ(polygon->distance(touchingDistance),0);
    EXPECT_DOUBLE_EQ(polygon->distance(SimplePolygonSamples::aaBB({4,0},{5,-1})),sqrt(2));
    EXPECT_DOUBLE_EQ(polygon->distance(SimplePolygonSamples::triangle({0.4,2.5},{0.6,2.5},{0.5,2.85})),0.15);
}

TEST_F(PolygonTest, equality) {
    EXPECT_EQ(*polygon,*polygon);
    Polygon<double> polygonOtherHoleOrder = {
        LinearRingSamples::COMPLEX_RING,
        {h2,h1}
    };
    EXPECT_EQ(*polygon,polygonOtherHoleOrder);
    EXPECT_NE(*polygon, Polygon<double>(LinearRingSamples::COMPLEX_RING));
    EXPECT_NE(*polygon, SimplePolygonSamples::aaBB({0,0},{1,1}));
}

TEST_F(PolygonTest, toString) {
    std::string expected = "Boundary: [(0.000000,4.000000),(2.000000,3.000000)],[(2.000000,3.000000),(2.000000,2.000000)],[(2.000000,2.000000),(4.000000,2.000000)],[(4.000000,2.000000),(2.000000,0.000000)],[(2.000000,0.000000),(-2.000000,-1.000000)],[(-2.000000,-1.000000),(0.000000,1.000000)],[(0.000000,1.000000),(-3.000000,2.000000)],[(-3.000000,2.000000),(0.000000,4.000000)]\nHoles: {[(0.000000,0.000000),(1.000000,1.000000)],[(1.000000,1.000000),(1.000000,0.000000)],[(1.000000,0.000000),(0.000000,0.000000)]},{[(0.000000,3.000000),(1.000000,3.000000)],[(1.000000,3.000000),(1.000000,2.000000)],[(1.000000,2.000000),(0.000000,2.000000)],[(0.000000,2.000000),(0.000000,3.000000)]}";
    EXPECT_EQ(polygon->toString(),expected);

}




