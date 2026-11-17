#include <cmath>
#include <gtest/gtest.h>
#include <fishnet/WGS84Ellipsoid.hpp>
#include <fishnet/Rectangle.hpp>
#include <fishnet/VectorLayer.hpp>
#include <fishnet/PathHelper.h>
#include <fishnet/TestUtil.hpp>
using namespace fishnet;
using namespace fishnet::geometry;

TEST(WGS84EllipsoidTest, flattening) {
    double flatteningExpected = 0.00335281066474748;
    EXPECT_DOUBLE_EQ(WGS84Ellipsoid::flattening,flatteningExpected);
}

TEST(WGS84EllipsoidTest, radius){
    double expectedRadiusKM = 6378.137;
    EXPECT_DOUBLE_EQ(WGS84Ellipsoid::radiusInMeter,expectedRadiusKM*1000);
}


TEST(WGS84EllipsoidTest, TokioBerlin){
    double phiBerlin = 52.516666666666667;
    double lambdaBerlin = 13.400;
    double phiTokio = 35.700;
    double lambdaTokio = 139.76666666666667;
    double distanceTokioBerlinInMeters = 8941202.50458698;
    EXPECT_DOUBLE_EQ(WGS84Ellipsoid::distance(lambdaBerlin,phiBerlin,lambdaTokio,phiTokio),distanceTokioBerlinInMeters);
    geometry::Vec2DReal berlin = {lambdaBerlin,phiBerlin};
    geometry::Vec2DReal tokio = {lambdaTokio, phiTokio};
    EXPECT_DOUBLE_EQ(WGS84Ellipsoid::distance(berlin,tokio),distanceTokioBerlinInMeters);
}

TEST(WGS84EllipsoidTest, IdenticalPointsHaveZeroDistance) {
    EXPECT_DOUBLE_EQ(WGS84Ellipsoid::distance(39.28, -6.8, 39.28, -6.8), 0.0);
    EXPECT_DOUBLE_EQ(WGS84Ellipsoid::distance(39.28, -6.8, 39.28, -6.8, false), 0.0);
    geometry::Vec2DReal p = {39.28, -6.8};
    EXPECT_DOUBLE_EQ(WGS84Ellipsoid::distance(p, p), 0.0);
}

TEST(WGS84EllipsoidTest, TokioBerlinApprox){
    double phiBerlin = 52.516666666666667;
    double lambdaBerlin = 13.400;
    double phiTokio = 35.700;
    double lambdaTokio = 139.76666666666667;
    double distanceTokioBerlinInMeters = 8928954.1420394;
    EXPECT_DOUBLE_EQ(WGS84Ellipsoid::distance(lambdaBerlin,phiBerlin,lambdaTokio,phiTokio, false),distanceTokioBerlinInMeters);
}

TEST(WGS84EllipsoidTest, NYCLondon){
    Vec2DReal nyc = {-74.006, 40.7128};
    Vec2DReal london = {-0.1278, 51.5074};
    double distance = 5585200.0;
    double actual = WGS84Ellipsoid::distance(nyc, london);
    EXPECT_NEAR(actual, distance, 1000.0) << "Distance between NYC and London should be approximately 5570 km, but was " << actual / 1000 << " km.";
}

TEST(WGS84EllipsoidTest, SquareKilometerArea){
    // Square polygon of approximately 1 km^2 (1000m x 1000m), given in WGS84 lon/lat degrees.
    double lat0 = 46.55;
    double lon0 = 11.87;
    double sideInMeters = 1000.0;
    double metersPerDegreeLat = 111320.0; // approximate meridian arc length per degree
    double dLat = sideInMeters / metersPerDegreeLat;
    double dLon = sideInMeters / (metersPerDegreeLat * std::cos(lat0 * fishnet::math::DEG_TO_RAD));

    geometry::Polygon<double> square = {Ring(std::vector<Vec2DReal>{
        {lon0, lat0},
        {lon0 + dLon, lat0},
        {lon0 + dLon, lat0 + dLat},
        {lon0, lat0 + dLat}
    })};

    double areaInSquareMeters = WGS84Ellipsoid::area(square);
    EXPECT_NEAR(areaInSquareMeters, 1'000'000.0, 5000.0) << "Expected area close to 1 km^2, but was " << areaInSquareMeters << " m^2.";
}