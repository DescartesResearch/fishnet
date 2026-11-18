#pragma once
#include <fishnet/IGeometry.hpp>
#include <fishnet/WGS84Ellipsoid.hpp>

struct WGS84Distance{
    static auto operator()(const fishnet::geometry::Vec2DReal & lhs, const fishnet::geometry::Vec2DReal & rhs) noexcept {
        return fishnet::WGS84Ellipsoid::distance(lhs,rhs);
    }
};

struct MetricDistance{
    static auto operator()(const fishnet::geometry::Vec2DReal & lhs, const fishnet::geometry::Vec2DReal & rhs) noexcept {
        return lhs.distance(rhs);
    }
};

using DistanceFunction = std::function<fishnet::math::DEFAULT_FLOATING_POINT(const fishnet::geometry::Vec2DReal &, const fishnet::geometry::Vec2DReal &)>;

static DistanceFunction distanceFunctionForSpatialReference(const OGRSpatialReference & spatialRef) {
    if(spatialRef.IsEmpty())
        throw std::runtime_error("No distance function available for empty coordinate system");
    // Projected coordinates (e.g. UTM northings around 1e7) carry rounding errors of about 1e-9, far above
    // fishnet::math::EPSILON (1e-12), so geometric predicates like Segment::contains reject computed points.
    // Unsupported until the tolerance scales with the coordinates; reproject to WGS84 (EPSG:4326) instead.
    if(spatialRef.IsProjected())
        throw std::runtime_error("Projected coordinate systems are not supported yet (\""+std::string(spatialRef.GetName())+"\"): reproject the input to WGS84 (EPSG:4326)");
    if(spatialRef.IsLocal())
        return MetricDistance();
    if(spatialRef.IsGeographic())
        return WGS84Distance();
    throw std::runtime_error("No distance function found for coordinate system: \""+std::string(spatialRef.GetName())+"\"");
}