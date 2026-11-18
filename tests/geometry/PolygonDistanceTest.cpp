#include <gtest/gtest.h>
#include <fishnet/Segment.hpp>
#include <fishnet/PolygonDistance.hpp>

using namespace fishnet::geometry;

// ~15 m building edge in lon/lat: the foot was rejected, so the nearer endpoint was returned
TEST(PolygonDistanceTest, ClosestPointOnShortLonLatSegmentIsFoot) {
    Segment<double> seg(Vec2DReal(39.2218724, -6.8284641), Vec2DReal(39.2219653, -6.8285636));
    Vec2DReal pt(39.2218961, -6.828499);
    auto foot = closestPointOnSegment(seg, pt);
    EXPECT_NEAR(foot.x, 39.2219008469421, 1e-12);
    EXPECT_NEAR(foot.y, -6.828494567930446, 1e-12);
    EXPECT_TRUE(seg.contains(foot));
}

// same geometry at the origin, worked before the fix
TEST(PolygonDistanceTest, ClosestPointOnShortSegmentAtOriginIsFoot) {
    Vec2DReal offset(39.2218724, -6.8284641);
    Segment<double> seg(Vec2DReal(39.2218724, -6.8284641) - offset, Vec2DReal(39.2219653, -6.8285636) - offset);
    Vec2DReal pt = Vec2DReal(39.2218961, -6.828499) - offset;
    auto foot = closestPointOnSegment(seg, pt);
    EXPECT_NEAR(foot.x, 39.2219008469421 - offset.x, 1e-12);
    EXPECT_NEAR(foot.y, -6.828494567930446 - offset.y, 1e-12);
}

// ~16 m building edge in UTM 37S metres: the rounding error of coordinates around 9e6 exceeded EPSILON,
// so the foot was rejected and the nearer endpoint was returned
TEST(PolygonDistanceTest, ClosestPointOnUtmSegmentIsFoot) {
    Segment<double> seg(Vec2DReal(530000.0, 9247000.0), Vec2DReal(530010.0, 9247012.0));
    Vec2DReal pt(530002.0, 9247009.0);
    auto foot = closestPointOnSegment(seg, pt);   // lambda = dot((2, 9), (10, 12)) / |(10, 12)|² = 128 / 244
    EXPECT_NEAR(foot.x, 530000.0 + 10.0 * 128.0 / 244.0, 1e-6);
    EXPECT_NEAR(foot.y, 9247000.0 + 12.0 * 128.0 / 244.0, 1e-6);
}

TEST(PolygonDistanceTest, ClosestPointBeyondSegmentIsEndpoint) {
    Segment<double> seg(Vec2DReal(0.0, 0.0), Vec2DReal(1.0, 0.0));
    EXPECT_EQ(closestPointOnSegment(seg, Vec2DReal(-2.0, 1.0)), Vec2DReal(0.0, 0.0));
    EXPECT_EQ(closestPointOnSegment(seg, Vec2DReal(3.0, -1.0)), Vec2DReal(1.0, 0.0));
    EXPECT_EQ(closestPointOnSegment(Segment<double>(Vec2DReal(2.0, 2.0), Vec2DReal(2.0, 2.0)), Vec2DReal(5.0, 5.0)), Vec2DReal(2.0, 2.0));
}
