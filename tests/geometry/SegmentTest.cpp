#include <gtest/gtest.h>
#include <cmath>
#include <fishnet/Segment.hpp>
#include <fishnet/TestUtil.hpp>

using namespace fishnet::geometry;
using namespace fishnet::math;
using namespace fishnet::test;

TEST(SegmentTest, init){
    Segment s {Vec2D(0,0),Vec2D(1,0)};
    EXPECT_EQ(s.p(), Vec2D(0,0));
    EXPECT_EQ(s.q(), Vec2D(1,0));
    EXPECT_TYPE<Vec2D<int>>(s.p());
    EXPECT_TYPE<Vec2D<int>>(s.q());
    Segment r {Vec2D(0.4,0.3),Vec2D(1L,2L)};
    EXPECT_EQ(r.p(), Vec2D(0.4,0.3));
    EXPECT_EQ(r.q(), Vec2D(1L,2L));
    EXPECT_TYPE<Vec2D<double>>(r.p());
    EXPECT_TYPE<Vec2D<double>>(r.q());
    Segment t {Vec2D(1,1),Vec2D(PI,PI)};
    EXPECT_EQ(t.p(), Vec2D(1,1));
    EXPECT_EQ(t.q(),Vec2D(PI,PI));
    EXPECT_TYPE<Vec2D<double>>(t.p());
    EXPECT_TYPE<Vec2D<double>>(t.q());
    Vec2D v {0,1};
    EXPECT_NO_FATAL_FAILURE(Segment(v,v));
}

TEST(SegmentTest, flip){
    Segment s {Vec2D(1,1),Vec2D(2,2)};
    EXPECT_EQ(s.flip(), Segment(Vec2D(2,2),Vec2D(1,1)));
    EXPECT_EQ(s.flip().flip(), s);
}

TEST(SegmentTest, direction){
    EXPECT_EQ(Segment(Vec2D(0,0),Vec2D(1,1)).direction(), Vec2D(1,1));
    EXPECT_EQ(Segment(Vec2D(),Vec2D()).direction(), Vec2D());
    EXPECT_EQ(Segment(Vec2D(2,1),Vec2D(-3,2)).direction(), Vec2D(-5,1));
}

TEST (SegmentTest, endpoints){
    Vec2D p = Vec2D(1,1);
    Vec2D q = Vec2D(1,2);
    Segment s {p,q};
    EXPECT_EQ(s.upperEndpoint(), q);
    EXPECT_EQ(s.lowerEndpoint(),p);
    EXPECT_EQ(s.leftEndpoint(),p);
    EXPECT_EQ(s.rightEndpoint(),q);
    q= Vec2D(2,1);
    Segment t = Segment(p,q);
    EXPECT_EQ(t.upperEndpoint(),q);
    EXPECT_EQ(t.lowerEndpoint(),p);
    EXPECT_EQ(t.leftEndpoint(),p);
    EXPECT_EQ(t.rightEndpoint(),q);
}

TEST(SegmentTest, length){
    EXPECT_DOUBLE_EQ(Segment(Vec2D(0,0),Vec2D(1,1)).length(),sqrt(2));
    EXPECT_EQ(Segment(Vec2D(1,0),Vec2D(-4,0)).length(),5);
    EXPECT_EQ(Segment(Vec2D(1,1),Vec2D(1,1)).length(),0);
}

TEST(SegmentTest, isValid){
    EXPECT_TRUE(Segment(Vec2D(1,1),Vec2D(0,0)).isValid());
    EXPECT_TRUE(Segment(Vec2D(0.123,0.456),Vec2D(100000L,2345578L)).isValid());
    EXPECT_FALSE(Segment(Vec2D(),Vec2D()).isValid());
    EXPECT_FALSE(Segment(Vec2D(1.01,1.0),Vec2D(1.01,1.0)).isValid());
    EXPECT_FALSE(Segment(Vec2D(0,0),Vec2D(0.1*DOUBLE_EPSILON,0.1*DOUBLE_EPSILON)).isValid());
}

TEST(SegmentTest, toLine){
    EXPECT_EQ(Segment(Vec2D(0,0),Vec2D(1,1)).toLine(),Line(1,0));
    EXPECT_EQ(Segment(Vec2D(-3,3),Vec2D(2,2)).toLine(),Line(-0.2,2.4));
}

TEST(SegmentTest, isParallel){
    Segment s {Vec2D(2,1),Vec2D(3,2)};
    EXPECT_TRUE(s.isParallel(s));
    EXPECT_TRUE(s.isParallel(Segment(Vec2D(0,0),Vec2D(1,1))));
    EXPECT_TRUE(s.isParallel(Line(1,0)));
    EXPECT_FALSE(s.isParallel(Segment(Vec2D(2,3),Vec2D(3,3))));
    EXPECT_FALSE(s.isParallel(Line(0,2)));
}

TEST(SegmentTest, intersects){
    Segment s {Vec2D(1,1),Vec2D(-2,2)};
    EXPECT_TRUE(s.intersects(Segment(Vec2D(0,0),Vec2D(0,3))));
    EXPECT_TRUE(s.intersects(Segment(Vec2D(2,2),Vec2D(-1,0))));
    EXPECT_TRUE(s.intersects(Y_AXIS));
    EXPECT_FALSE(s.intersects(X_AXIS));
    EXPECT_FALSE(s.intersects(Segment(Vec2D(-3,-1),Vec2D(2,1))));
}

TEST(SegmentTest, overlaps){
    Segment s {Vec2D(0,0),Vec2D(2,2)};
    EXPECT_TRUE(s.overlaps(Segment(Vec2D(-1,-1),Vec2D(0.5,0.5))));
    EXPECT_TRUE(s.overlaps(s));
    EXPECT_TRUE(s.overlaps(Segment(Vec2D(2,2),Vec2D(3,3))));
    EXPECT_FALSE(s.overlaps(Segment(Vec2D(2,2),Vec2D(3,2)))); //not parallel
    EXPECT_FALSE(s.overlaps(Segment(Vec2D(3,3),Vec2D(4,4)))); // no common points
    EXPECT_FALSE(s.overlaps(Segment(Vec2D(-1,-1),Vec2D(-1,0))));
}

TEST(SegmentTest, containsSegment){
    Segment s {Vec2D(0,0),Vec2D(2,2)};
    Segment t {Vec2D(0,0),Vec2D(1,1)};
    EXPECT_TRUE(s.contains(t));
    EXPECT_FALSE(t.contains(s));
    EXPECT_TRUE(s.contains(Segment(Vec2D(0.5,0.5),Vec2D(1.2,1.2))));
    EXPECT_FALSE(s.contains(Segment(Vec2D(0,0),Vec2D(1,2))));
}

TEST(SegmentTest, touches){
    Segment s {Vec2D(0,0),Vec2D(2,2)};
    EXPECT_TRUE(s.touches(Segment(Vec2D(0,0),Vec2D(-1,0))));
    EXPECT_TRUE(s.touches(Segment(Vec2D(2,2),Vec2D(-1000.1,0))));
    EXPECT_FALSE(s.touches(s));
    EXPECT_FALSE(s.touches(Segment(Vec2D(0,0),Vec2D(3,3))));
    EXPECT_FALSE(s.touches(Segment(Vec2D(-2,-2),Vec2D(1,1))));
    EXPECT_FALSE(s.touches(Segment(Vec2D(-1,-1),Vec2D(2,2))));
    EXPECT_FALSE(s.touches(Segment(Vec2D(-100.23,0),Vec2D(0,1000.3))));
}

TEST(SegmentTest, equality){
    Segment s {Vec2D(1,0),Vec2D(2,0)};
    EXPECT_EQ(s,s);
    EXPECT_EQ(s, Segment(Vec2D(1,0),Vec2D(2,0)));
    EXPECT_EQ(Segment(Vec2D(-PI,PI),Vec2D(0,0)),Segment(Vec2D(-PI,PI),Vec2D()));
    EXPECT_EQ(Segment(Vec2D(0.0,0.0),Vec2D(1L,1L)),Segment<float>(Vec2D<float>(0,0),Vec2D(1.0f,1.0f)));
    EXPECT_EQ(s, Segment(Vec2D(2,0),Vec2D(1,0)));
    EXPECT_NE(s, Segment(Vec2D(1,0),Vec2D(2,0.5)));
    EXPECT_NE(s, Segment(Vec2D(2,0),Vec2D(0,0)));
    auto hasher = std::hash<Segment<int>>{};
    EXPECT_EQ(hasher(s),hasher(s));
    EXPECT_EQ(hasher(s),hasher(Segment(Vec2D(2,0),Vec2D(1,0))));
    EXPECT_NE(hasher(s),hasher(Segment(Vec2D(2,0),Vec2D(0,0))));
}

TEST(SegmentTest, contains){
    Segment s {Vec2D(0,0),Vec2D(3,2)};
    EXPECT_TRUE(s.contains(Vec2D(s.p())));
    EXPECT_TRUE(s.contains(Vec2D(s.q())));
    EXPECT_TRUE(s.contains(Vec2D(1.5,1)));
    EXPECT_TRUE(s.contains(Vec2D(1,2.0/3.0)));
    EXPECT_FALSE(s.contains(Vec2D(1,1)));
    EXPECT_FALSE(s.contains(Vec2D(-1.5,-1)));
    EXPECT_TRUE(Segment(Vec2D(),Vec2D()).contains(Vec2D()));
    EXPECT_FALSE(Segment(Vec2D(1,1),Vec2D(1,1)).contains(Vec2D(0.5,0.5)));
}

TEST(SegmentTest, distance){
    Segment s {Vec2D(0,0),Vec2D(2,2)};
    EXPECT_EQ(s.distance(Vec2D(0,0)),0);
    EXPECT_EQ(s.distance(Vec2D(2,2)),0);
    EXPECT_DOUBLE_EQ(s.distance(Vec2D(0,1)),sqrt(2)/2);
    EXPECT_EQ(Segment(Vec2D(0,0),Vec2D(10,0)).distance(Vec2D(5,0.5)),0.5);
    EXPECT_TRUE(areEqual(Segment(Vec2D(2,2),Vec2D(4,2)).distance(Vec2D(5,4)), 2.2360679774998));
}

TEST(SegmentTest, distanceSegment){
    Segment s {Vec2D(0,0),Vec2D(2,2)};
    Segment t {Vec2D(1.5,0.5),Vec2D(3,1)};
    EXPECT_DOUBLE_EQ(s.distance(t),sqrt(2)/2);
    EXPECT_DOUBLE_EQ(s.distance(t),sqrt(2)/2);
    Segment r {Vec2D(3,2),Vec2D(3.5,4)};
    EXPECT_DOUBLE_EQ(s.distance(r),1);
    EXPECT_DOUBLE_EQ(r.distance(s),1);
    EXPECT_DOUBLE_EQ(r.distance(t),1);
    EXPECT_DOUBLE_EQ(t.distance(r),1);   
}

TEST(SegmentTest, intersection){
    Segment s {Vec2D(0,0),Vec2D(3,2)};
    Segment t {Vec2D(1,2),Vec2D(2,0)};
    auto inter = s.intersection(t);
    EXPECT_VALUE(inter,Vec2D(1.5,1));
    EXPECT_VALUE(s.intersection(X_AXIS),Vec2D(0,0));
    EXPECT_VALUE(s.intersection(Line(-4,14)),Vec2D(3,2));
    EXPECT_EQ(s.intersection(Segment(Vec2D(-2,1),Vec2D(0,1))),std::nullopt);
    EXPECT_TRUE(s.intersection(Line(Vec2D(-2,1),Vec2D(0,1))).has_value());
    EXPECT_EQ(s.intersection(Line(2,2)),std::nullopt);
}

TEST(SegmentTest, toString){
    Segment s{Vec2D(0,0),Vec2D(3,2)};
    EXPECT_EQ(s.toString(),"[(0,0),(3,2)]");
    std::cout << "Test console output for Segment: " << s << std::endl;
}
/**
 * @brief contains() must agree with Vec2D::operator== about a point sitting on an endpoint
 *
 * contains() works on a lambda normalised over the segment, so a tolerance applied to that lambda
 * means EPSILON * length in coordinate space. On the segments of a WGS84 building footprint the
 * direction is ~9e-5 degrees, which used to shrink the effective tolerance to ~9e-17 - below one
 * ulp of a coordinate near 50. A point one ulp past the endpoint, as produced by an intersection,
 * was then reported as not contained even though Vec2D still called it equal to that endpoint.
 * Ring's ray casting miscounted the crossing and reported interior points as outside.
 */
TEST(SegmentTest, containsPointWhichEqualsAnEndpoint){
    Segment vertical {Vec2D(9.9595317235047229, 49.827392684956379), Vec2D(9.9595317235047229, 49.827482684956379)};
    Vec2D justPastEnd {vertical.q().getX(), std::nextafter(vertical.q().getY(), 100.0)};
    ASSERT_EQ(justPastEnd, vertical.q()) << "the two points are equal as far as Vec2D is concerned";
    EXPECT_TRUE(vertical.contains(justPastEnd)) << "so the segment has to contain it as well";

    Segment horizontal {Vec2D(9.9595317235047229, 49.827482684956379), Vec2D(9.9596217235047229, 49.827482684956379)};
    Vec2D justPastRight {std::nextafter(horizontal.q().getX(), 100.0), horizontal.q().getY()};
    ASSERT_EQ(justPastRight, horizontal.q());
    EXPECT_TRUE(horizontal.contains(justPastRight));
}

// ~15 m building edge in lon/lat
TEST(SegmentTest, ContainsPointOnShortLonLatSegment) {
    Segment<double> seg(Vec2DReal(39.2218724, -6.8284641), Vec2DReal(39.2219653, -6.8285636));
    EXPECT_TRUE(seg.contains(Vec2DReal(39.2219008469421, -6.828494567930446)));
}

TEST(SegmentTest, DoesNotContainPointNextToShortLonLatSegment) {
    Segment<double> seg(Vec2DReal(39.2218724, -6.8284641), Vec2DReal(39.2219653, -6.8285636));
    Vec2DReal foot(39.2219008469421, -6.828494567930446);
    EXPECT_FALSE(seg.contains(foot + Vec2DReal(1e-9, 0)));   // ~0.1 mm beside
    EXPECT_FALSE(seg.contains(Vec2DReal(39.2219653, -6.8285636) + (Vec2DReal(39.2219653, -6.8285636) - Vec2DReal(39.2218724, -6.8284641)) * 1e-6)); // just past q
}

// endpoints equal per Vec2D::operator==
TEST(SegmentTest, ContainsOnSegmentShorterThanEpsilon) {
    Segment<double> tiny(Vec2DReal(39.28, -6.8), Vec2DReal(39.28 + 1e-13, -6.8));
    EXPECT_TRUE(tiny.contains(Vec2DReal(39.28, -6.8)));
    EXPECT_FALSE(tiny.contains(Vec2DReal(39.29, -6.8)));
}

// ~1 m edges in lon/lat crossing at 0.3 degrees were treated as parallel, so the crossing was missed
TEST(SegmentTest, IntersectsNearlyParallelShortLonLatSegment) {
    double angle = 0.3 * PI / 180;
    Vec2DReal mid {39.280005, -6.8};
    Vec2DReal half {5e-6 * std::cos(angle), 5e-6 * std::sin(angle)};
    Segment<double> a {Vec2DReal(39.28, -6.8), Vec2DReal(39.28001, -6.8)};
    Segment<double> b {mid - half, mid + half};
    ASSERT_TRUE(a.intersects(b));
    EXPECT_NEAR(a.intersection(b).value().x, mid.x, 1e-12);
    EXPECT_NEAR(a.intersection(b).value().y, mid.y, 1e-12);
}
