#include <gtest/gtest.h>
#include <fishnet/Ray.hpp>
#include <fishnet/TestUtil.hpp>
#include <fishnet/Segment.hpp>

using namespace fishnet::geometry;
using namespace fishnet::test;

TEST(RayTest, init){
    Ray r {Vec2D(1,1),Vec2D(1,0)};
    EXPECT_EQ(r.origin(), Vec2D(1,1));
    EXPECT_EQ(r.direction(),Vec2D(1,0));
    EXPECT_EQ(Ray(Vec2D(-2.4,0),Vec2D(1,1)).direction(),Vec2D(1,1));
    auto s = Ray(Vec2D(2,1),Vec2D(-1,2.123));
    EXPECT_TYPE<Ray<double>>(s);
    EXPECT_EQ(s.origin(),Vec2D(2,1));
    EXPECT_EQ(s.direction(),Vec2D(-1,2.123));
    EXPECT_TYPE<Vec2D<double>>(s.origin());
    auto t = Ray(Vec2D(3.14,-1.0f),Vec2D(2L,1L));
    EXPECT_TYPE<Ray<double>>(t);
    EXPECT_EQ(t.direction(),Vec2D(2,1));
    auto u = Ray(Vec2D(0,0),Vec2D(1,0));
    EXPECT_TYPE<Ray<int>>(u);
    EXPECT_ANY_THROW(Ray(Vec2D(0,1),Vec2D(0,0)));
}

TEST(RayTest, verticalAndHorizontalRays){
    auto verticalUp = Ray<int>::up(Vec2D(1,1));
    EXPECT_EQ(verticalUp.origin(),Vec2D(1,1));
    EXPECT_EQ(verticalUp.direction(), Vec2D(0,1));
    EXPECT_TRUE(verticalUp.isParallel(Y_AXIS));
    auto horizontalRight = Ray<double>::right(Vec2D(0.5,0.2));
    EXPECT_EQ(horizontalRight.origin(), Vec2D(0.5,0.2));
    EXPECT_EQ(horizontalRight.direction(),Vec2D(1,0));
    EXPECT_TRUE(horizontalRight.isParallel(X_AXIS));
    auto down = Ray<double>::down(Vec2D(2,3));
    EXPECT_EQ(down.origin(), Vec2D(2,3));
    EXPECT_EQ(down.direction(),Vec2D(0,-1));
    EXPECT_TRUE(down.isParallel(Y_AXIS));
    auto left = Ray<float>::left(Vec2D(-1,-2));
    EXPECT_EQ(left.origin(),Vec2D(-1,-2));
    EXPECT_EQ(left.direction(),Vec2D(-1,0));
    EXPECT_TRUE(left.isParallel(X_AXIS));
}   

TEST(RayTest, toLine){
    Ray r {Vec2D(0,0),Vec2D(1,0)};
    auto l = r.toLine();
    EXPECT_EQ(l.p(), r.origin());
    EXPECT_EQ(l.q() , r.direction()+r.origin());
    EXPECT_EQ(l.q(), Vec2D(1,0));
    EXPECT_TRUE(l.isParallel(r));
    auto s = Ray(Vec2D(-2,1),Vec2D(2,1));
    auto m = s.toLine();
    EXPECT_EQ(m.q(), Vec2D(0,2));
    EXPECT_EQ(s.direction(),m.direction());
}

TEST(RayTest, oppositeRay){
    EXPECT_EQ(Ray<int>::up(Vec2D(0,0)).oppositeRay(), Ray<int>::down(Vec2D(0,0)));
    Ray r {Vec2D(2,1),Vec2D(1,1)};
    auto rFlipped = r.oppositeRay();
    EXPECT_EQ(rFlipped.origin(),Vec2D(2,1));
    EXPECT_EQ(rFlipped.direction(),Vec2D(-1,-1));
    EXPECT_EQ(- Ray<double>::right(Vec2D(3,-1)), Ray<int>::left(Vec2D(3,-1)));
}

TEST(RayTest, isParallel){
    EXPECT_TRUE(Ray<int>::up(Vec2D(0,0.5)).isParallel(Ray<int>::down(Vec2D(-2,3))));
    EXPECT_FALSE(Ray(Vec2D(2,2),Vec2D(-1,0.25)).isParallel(Ray(Vec2D(2,2),Vec2D(1,1))));
    EXPECT_TRUE(Ray<int>::right(Vec2D(1,1)).isParallel(X_AXIS));
    EXPECT_FALSE(Ray<double>::left(Vec2D(-2.3,-1)).isParallel(Y_AXIS));
    EXPECT_FALSE(Ray(Vec2D(1,1),Vec2D(1,2)).isParallel(Segment(Vec2D(1,1),Vec2D(2,1))));
    EXPECT_TRUE(Ray(Vec2D(0,0),Vec2D(1,2)).isParallel(Segment(Vec2D(1,1),Vec2D(2,3))));
}

TEST(RayTest, intersects){
    EXPECT_TRUE(Ray<int>::up(Vec2D(-1,-1)).intersects(Ray(Vec2D(-2,-1),Vec2D(1,1))));
    EXPECT_TRUE(Ray<int>::up(Vec2D(-1,-1)).intersects(X_AXIS));
    EXPECT_FALSE(Ray(Vec2D(1,1),Vec2D(1,2)).intersects(Ray(Vec2D(1,0.5),Vec2D(1,1))));
    EXPECT_FALSE(Ray<int>::up(Vec2D(1,1)).intersects(X_AXIS));
    Ray r {Vec2D(1,1),Vec2D(1,1)};
    EXPECT_FALSE(r.intersects(Y_AXIS));
    EXPECT_TRUE(r.intersects(Segment(Vec2D(4,5),Vec2D(6,5))));
    EXPECT_FALSE(r.intersects(Segment(Vec2D(3,5),Vec2D(4,5))));
    EXPECT_FALSE(r.intersects(r));
}

TEST(RayTest, contains){
    Ray r {Vec2D(1,1),Vec2D(1,2)};
    EXPECT_TRUE(r.contains(r.origin()));
    EXPECT_TRUE(r.contains(Vec2D(2,3)));
    EXPECT_TRUE(r.contains(Vec2D(4,7)));
    EXPECT_FALSE(r.contains(Vec2D()));
    EXPECT_FALSE(r.contains(Vec2D(0,-1)));
    EXPECT_TRUE(Ray<int>::up(Vec2D(-1,-2)).contains(Vec2D(-1,0)));
}

TEST(RayTest, equality){
    Ray r {Vec2D(1,1),Vec2D(1,2)};
    std::hash<Ray<int>> hasher {};
    EXPECT_EQ(r,r);
    EXPECT_EQ(r, Ray(Vec2D(1,1),Vec2D(1,2).normalize()));
    EXPECT_NE(r, Ray(Vec2D(0,1),Vec2D(1,2)));
    EXPECT_NE(r, Ray<int>::left(Vec2D(1,1)));
    EXPECT_NE(r, Ray(Vec2D(2,3),Vec2D(1,2)));
    EXPECT_EQ(r, Ray(Vec2D(1.0f,1.0f),Vec2D(1.0f,2.0f)));
    EXPECT_EQ(hasher(r),hasher(r));
    EXPECT_EQ(hasher(r),hasher(Ray(Vec2D(1,1),Vec2D(2,4))));
}

TEST(RayTest, intersection){
    Ray r {Vec2D(-1,1),Vec2D(1,1)};
    auto yIntercection = r.intersection(Y_AXIS);
    EXPECT_EQ(yIntercection.value(), Vec2D(0,2));
    EXPECT_EQ(r.intersection(X_AXIS),std::nullopt);
    EXPECT_EQ(r.intersection(Ray<int>::up(Vec2D(2,0))).value(), Vec2D(2,4));
    EXPECT_EQ(r.intersection(Ray<int>::up(Vec2D(1,10))),std::nullopt);
    EXPECT_EQ(r.intersection(Segment(Vec2D(0,4),Vec2D(2,2))).value(),Vec2D(1,3));
}

TEST(RayTest, toString){
    Ray r {Vec2D(-1,1),Vec2D(1,1)};
    EXPECT_EQ(r.toString(),"Ray (-1,1) + k * (1,1)");
    std::cout << "Testing Console Output: " << r << std::endl;
}
// ~15 m building edge in lon/lat
TEST(RayTest, ContainsPointOnShortLonLatRay) {
    Ray<double> ray {Vec2DReal(39.2218724, -6.8284641), Vec2DReal(0.0000929, -0.0000995)};
    Vec2DReal foot(39.2219008469421, -6.828494567930446);
    EXPECT_TRUE(ray.contains(foot));
    EXPECT_TRUE(ray.contains(Vec2DReal(39.2218724, -6.8284641) + Vec2DReal(0.0000929, -0.0000995) * 3.0));
    EXPECT_FALSE(ray.contains(foot + Vec2DReal(1e-9, 0)));   // ~0.1 mm beside
    EXPECT_FALSE(ray.contains(Vec2DReal(39.2218724, -6.8284641) - Vec2DReal(0.0000929, -0.0000995) * 1e-6)); // just behind the origin
}
