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
