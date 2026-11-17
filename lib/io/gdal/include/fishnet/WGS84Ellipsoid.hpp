#pragma once
#include <math.h>
#include <fishnet/Constants.hpp>
#include <fishnet/Radians.hpp>
#include <fishnet/Degrees.hpp>
#include <fishnet/Angle.hpp>
#include <fishnet/IGeometry.hpp>
#include <fishnet/OGRGeometryAdapter.hpp>
#include "gdal/ogr_spatialref.h"

namespace fishnet {

/**
 * @brief Utility class for computing distances and areas in metric units for GIS data using the WGS84 Spatial Reference System
 * 
 */
class WGS84Ellipsoid {
private:
    static double cos2(double angle) {
        return pow(cos((math::PI / 180) * angle), 2);
    }

    static double cos2(math::Degrees angle){
        return pow(angle.cos(), 2);
    }

    static double sin2(double angle) {
        return pow(sin((math::PI / 180) * angle), 2);
    }

    static double sin2(math::Degrees angle){
        return pow(angle.sin(), 2);
    }

    static auto projectToMetric(fishnet::geometry::Shape auto const & shape) {
        using T = std::remove_cvref_t<decltype(shape)>;
        auto centroid = shape.centroid();
        OGRSpatialReference metric;
        metric.SetAE(centroid.y, centroid.x, 0, 0);
        metric.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
        fishnet::geometry::OGRGeometryAdapter asOGR(shape);
        OGRCoordinateTransformation * toMetric = OGRCreateCoordinateTransformation(&spatialReference, &metric);
        if(toMetric == nullptr || asOGR.raw()->transform(toMetric) != OGRERR_NONE)
            throw std::runtime_error("Could not transform geometry to metric");
        OCTDestroyCoordinateTransformation(OGRCoordinateTransformation::ToHandle(toMetric));
        return std::move(asOGR).narrowTo<T>().value_or_throw();
    }

    static inline OGRSpatialReference initWGS84(){
        OGRSpatialReference wgs84 = OGRSpatialReference();
        wgs84.importFromEPSG(4326);
        wgs84.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
        return wgs84;
    }

public:
    constexpr static double flattening = 1.0 / 298.257223563; // Flattening of the Earth: https://en.wikipedia.org/wiki/Earth_ellipsoid
    constexpr static double radiusInMeter = 6378137; //Earth radius in m;
    static inline OGRSpatialReference spatialReference = initWGS84();

    /**
     *
     * @param lambdaA longitude of point A
     * @param phiA latitude of point A
     * @param lambdaB longitude of point B
     * @param phiB latitude of point B
     * @param exact applies additional correction for the distances
     * For further reading consider: https://de.wikipedia.org/wiki/Orthodrome
     * @return distance between A and B in meters
     */
    static double distance(double lambdaA, double phiA, double lambdaB, double phiB, bool exact = true) {
        math::Degrees F = math::Degrees((phiA + phiB) / 2);
        math::Degrees G = math::Degrees((phiA - phiB) / 2);
        math::Degrees l = math::Degrees((lambdaA - lambdaB) / 2);
        double S = sin2(G) * cos2(l) + cos2(F) * sin2(l);
        if (S == 0.0) {
            // identical points: w becomes 0, so T = 0/0 = NaN and h2 divides by S = 0 in the exact branch
            return 0.0;
        }
        double C = cos2(G) * cos2(l) + sin2(F) * sin2(l);
        math::Radians w = math::Radians::atan(sqrt(S / C));
        double distance = 2 * w.getAngleValue() * radiusInMeter;
        if (not exact) {
            return distance;
        } else {
            double T = sqrt(S * C) / w.getAngleValue();
            double h1 = (3 * T - 1) / (2 * C);
            double h2 = (3 * T + 1) / (2 * S);
            double correction = (1 + flattening * h1 * sin2(F) * cos2(G) -
                                 flattening * h2 * cos2(F) * sin2(G));
            return distance * correction;
        }
    }

    /**
     * @brief Compute the distance between two points in metric units
     * 
     * @param p point (long,lat)
     * @param q point (long,lat)
     * @param exact applies additional correction for the distances
     * @return distance between p and q in meters
     */
    static double distance(fishnet::geometry::IPoint auto const & p, fishnet::geometry::IPoint auto const & q,bool exact = true)noexcept{
        return distance(p.x, p.y, q.x, q.y, exact);
    }

    /**
     * Calculate the area of a Shape in m² by projection
     * @param shape Shape to calculate the area of
     * @deprecated
     * @throws runtime_error when the projection to metric fails
     * @return area in m²
     */
    static double area(fishnet::geometry::Shape auto const & shape){
        return projectToMetric(shape).area();
    }
};
}
