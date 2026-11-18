#include <gtest/gtest.h>
#include <fishnet/Vec2D.hpp>
#include <fishnet/TestUtil.hpp>
#include <algorithm>
using namespace fishnet::test;
using namespace fishnet::geometry;
using namespace fishnet::math;

TEST(Vec2DTest, init){
    [[maybe_unused]] auto x = Vec2D<int>(1,1);
    EXPECT_TYPE<int>(x.x);
    [[maybe_unused]] auto y = Vec2D<float>(1.1f,1.1f);
    EXPECT_TYPE<float>(y.y);
    [[maybe_unused]] auto z = Vec2D(1.41,1.41);
    [[maybe_unused]] auto w = Vec2D<long>(123456789,123456789);
    auto d = Vec2D<double>(3.14,3.14);
    auto copyCons = Vec2D<double>(d);
    EXPECT_EQ(d,copyCons);
    // Implicit conversion constructors (only one parameter is a double)
    EXPECT_EQ(Vec2D(0,0.0),Vec2D(0.0,0.0));
    EXPECT_EQ(Vec2D(0.0,0),Vec2D(0.0,0.0));
}

TEST(Vec2DTest, negate){
    auto v = Vec2D<int>(1,1);
    EXPECT_EQ(-v, Vec2D(-1,-1));
}

TEST(Vec2DTest, plusMinus){
    auto r = Vec2D<int>(1,0);
    auto s = Vec2D<int>(0,1);
    auto sum = r + s;
    EXPECT_EQ(sum.x,1);
    EXPECT_EQ(sum.y,1);
    EXPECT_EQ(Vec2D<double>(1,1.0)+Vec2D<double>(1.5,0.0), Vec2D(2.5,1.0));
    EXPECT_EQ(Vec2D(0,0)-Vec2D(1,1.0),Vec2D(-1.0,-1.0));
}

TEST(Vec2DTest, mulDiv){
    EXPECT_EQ(Vec2D(1,1)*2.5,Vec2D(2.5,2.5));
    EXPECT_EQ(Vec2D(1.5,1.5)*2, Vec2D(3,3));
    EXPECT_EQ(Vec2D(2,2) / 2, Vec2D(1.0,1.0));
    EXPECT_EQ(Vec2D(3.14,1) / 0, Vec2D(3.14,1));
    EXPECT_EQ(Vec2D(1,1) * -1.1, Vec2D(-1.1,-1.1));
}

TEST(Vec2DTest, equality){
    EXPECT_EQ(Vec2D(1,1),Vec2D(1,1));
    EXPECT_EQ(Vec2D(1,1),Vec2D(1.0,1.0));
    EXPECT_EQ(Vec2D(1.0,1.0), Vec2D(1.0f,1.0f));
    EXPECT_EQ(Vec2D(1,1),Vec2D(1.0f,1.0f));
    EXPECT_EQ(Vec2D(1,1),Vec2D(1L,1L));
    EXPECT_NE(Vec2D(1.0,1.0),Vec2D(1.0,1.01));
    EXPECT_NE(Vec2D(0.0,0.0),Vec2D(DOUBLE_EPSILON,DOUBLE_EPSILON));
    EXPECT_EQ(Vec2D(),Vec2D(0.0,0.0));
    EXPECT_EQ(Vec2D(),Vec2D(DOUBLE_EPSILON*0.1,DOUBLE_EPSILON*0.1));
    EXPECT_EQ(Vec2D(0.0f,0.0f),Vec2D(1e-7f,1e-7f));
    EXPECT_NE(Vec2D(0.0f,0.0f),Vec2D(1e-4f,1e-4f));
}

TEST(Vec2DTest, hash){
    auto hasher = std::hash<Vec2D<double>>{};
    EXPECT_EQ(hasher(Vec2D(1.0,1.0)),hasher(Vec2D(1.0,1.0)));
    EXPECT_NE(hasher(Vec2D(1.0,2.0)),hasher(Vec2D(2.0,1.0)));
    auto intHasher = std::hash<Vec2D<int>> {};
    EXPECT_EQ(intHasher(Vec2D(1,2)),hasher(Vec2D(1.0,2.0)));
}

TEST(Vec2DTest, dot){
    Vec2D v {1,1};
    Vec2D w {-1,1};
    EXPECT_EQ(v.dot(w), 0);
    EXPECT_EQ(Vec2D(1,0).dot(Vec2D(-0.5,1)),-0.5);
    EXPECT_EQ(Vec2D(2,2).dot(Vec2D(2,2)),8);
    auto i = Vec2D(1,1).dot(Vec2D(1,1));
    bool typeMatch = std::same_as<int,decltype(i)>;
    EXPECT_TRUE(typeMatch);
}

TEST(Vec2DTest, cross){
    Vec2D v {1,1};
    Vec2D w {-1,1};
    EXPECT_EQ(v.cross(w),2);
    EXPECT_EQ(Vec2D(1.0,1.41).cross(Vec2D(1.0,1.41)), 0);
    EXPECT_EQ(Vec2D(5.0,2.0).cross(Vec2D(1.0,-2.0)),-12.0);
    auto i = Vec2D(1,1).cross(Vec2D(1,1));
    bool typeMatch = std::same_as<int,decltype(i)>;
    EXPECT_TRUE(typeMatch);
}

TEST(Vec2DTest, isParallel){
    EXPECT_TRUE(Vec2D(1,0).isParallel(Vec2D(2,0)));
    EXPECT_TRUE(Vec2D(1,1).isParallel(Vec2D(2,2)));
    EXPECT_FALSE(Vec2D(-1,0).isParallel(Vec2D(1,1)));
    EXPECT_TRUE(Vec2D().isParallel(Vec2D(10000.0,10000.0)));
    EXPECT_TRUE(Vec2D(1,0).isParallel(Vec2D(-1,0)));
}

TEST(Vec2DTest, isOrthogonal){
    EXPECT_TRUE(Vec2D(1,0).isOrthogonal(Vec2D(0,1)));
    EXPECT_TRUE(Vec2D(3.14,0).isOrthogonal(Vec2D(0,1)));
    EXPECT_TRUE(Vec2D(-1,1).isOrthogonal(Vec2D(1,1)));
    EXPECT_FALSE(Vec2D(1,2).isOrthogonal(Vec2D(2,1)));
    EXPECT_FALSE(Vec2D(1.0,0.001).isOrthogonal(Vec2D(0,1)));
}

TEST(Vec2DTest, length){
    EXPECT_DOUBLE_EQ(Vec2D(1,0).length(),1.0);
    Vec2D v {1,1};
    EXPECT_DOUBLE_EQ(v.length(), sqrt(2));
    EXPECT_DOUBLE_EQ(Vec2D(3,4).length(),5);
}

TEST(Vec2DTest, distance){
    double dist = Vec2D(1,1).distance(Vec2D(0,0));
    EXPECT_DOUBLE_EQ(dist, sqrt(2));
    EXPECT_DOUBLE_EQ(Vec2D(1,0).distance(Vec2D(1,1)),1.0);
    Vec2D v {3.14,3.14};
    EXPECT_DOUBLE_EQ(v.distance(v),0.0);
}

TEST(Vec2DTest, orthogonal){
    Vec2D v {1,0};
    auto w = v.orthogonal();
    EXPECT_TRUE(v.isOrthogonal(w));
    EXPECT_EQ(w, Vec2D(0,-1));
    auto z = Vec2D(2.3456,1.099887);
    EXPECT_TRUE(z.orthogonal().isOrthogonal(z));
}

TEST(Vec2DTest, normalize){
    Vec2D v {1,0};
    EXPECT_EQ(v.normalize(),v);
    EXPECT_EQ(v.normalize().length(),1.0);
    EXPECT_EQ(Vec2D(1,1).normalize(),Vec2D(1/sqrt(2),1/sqrt(2)));
    EXPECT_EQ(Vec2D().normalize(), Vec2D());
    EXPECT_EQ(Vec2D(12345,9887).normalize().length(),1.0);
}

TEST(Vec2DTest, toString){
    EXPECT_EQ(Vec2D(1,2).toString(), "(1,2)");
    std::cout <<"Test console output for Vec2D(1,1): " << Vec2D(1,1) << std::endl;
}

TEST(Vec2DTest, angle){
    Vec2D v {1,1};
    Vec2D ref {0,0};
    EXPECT_EQ(v.angle(ref),Radians(PI/4));
    EXPECT_EQ(Vec2D(1,0).angle(ref),Radians(0));
    EXPECT_EQ(Vec2D(-1,0).angle(ref),Radians::PI);
    EXPECT_EQ(Vec2D(0,-1).angle(ref).toDegrees(),Degrees(270.0));
    EXPECT_EQ(Vec2D(1,1).angle(Vec2D(1,0)),Radians(PI/2));
}

TEST(Vec2DTest, anlgeRotate){
    Vec2D ref {0,0};
    EXPECT_EQ(Vec2D(1,0).angle(ref,Radians(PI/2)),Radians(PI/2));
    EXPECT_EQ(Vec2D(0,-1).angle(ref,Radians::PI),Radians(PI/2));
}

TEST(Vec2DTest, LexicographicOrdering){
    std::vector<Vec2D<double>> points = {
        {2,1},{1,1},{-2,5},{-2,3},{0,0},{0,0.00004},{3,3},{3,2}
    };
    std::vector<Vec2D<double>> expected = {
        {-2,3},{-2,5},{0,0},{0,0.00004},{1,1},{2,1},{3,2},{3,3}  
    };
    std::ranges::sort(points,LexicographicOrder{});
    EXPECT_SORTED_RANGE_EQ(points,expected);
}


// ~1 m and ~10 cm edges in lon/lat: |cross| is tiny because the vectors are short, not because they are parallel
TEST(Vec2DTest, isParallelShortLonLatVectors) {
    double angle = 0.3 * PI / 180;
    Vec2D a {1e-5, 0.0};
    EXPECT_FALSE(a.isParallel(Vec2D(1e-5 * std::cos(angle), 1e-5 * std::sin(angle))));
    EXPECT_FALSE(Vec2D(1e-6, 0.0).isParallel(Vec2D(0.0, 1e-6)));
    EXPECT_TRUE(a.isParallel(Vec2D(3e-5, 0.0)));
    EXPECT_TRUE(Vec2D(0.0000929, -0.0000995).isParallel(Vec2D(0.0000929, -0.0000995) * 2.5));
}

// ~1 m vectors in lon/lat: |dot| is tiny because the vectors are short
TEST(Vec2DTest, isOrthogonalShortLonLatVectors) {
    double angle = 89.7 * PI / 180;
    Vec2D a {1e-5, 0.0};
    EXPECT_FALSE(a.isOrthogonal(Vec2D(1e-5 * std::cos(angle), 1e-5 * std::sin(angle))));
    EXPECT_TRUE(a.isOrthogonal(Vec2D(0.0, 3e-5)));
    EXPECT_TRUE(Vec2D(0.0000929, -0.0000995).isOrthogonal(Vec2D(0.0000929, -0.0000995).orthogonal()));
}
