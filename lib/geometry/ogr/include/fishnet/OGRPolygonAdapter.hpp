#pragma once
#include <vector>
#include <fishnet/IGeometry.hpp>
#include <fishnet/Rectangle.hpp>
#include <fishnet/Polygon.hpp>
#include <fishnet/PolygonDistance.hpp>
#include "OGRPolygonalAdapter.hpp"
#include "OGRRingAdapter.hpp"

namespace fishnet::geometry{

/**
 * @brief IPolygon implementation backed by an OGRPolygon
 *
 * The exterior ring forms the boundary, the interior rings are the holes. Where OGR/OGC and
 * fishnet disagree, fishnet's convention wins, so that an OGR backed polygon is substitutable for
 * a fishnet::geometry::Polygon:
 * - contains() includes the boundary, whereas OGR's Contains() is interior only
 * - centroid() is the area weighted centroid over the mean-of-points centroids of its rings
 * - distance() returns 0 for polygons which overlap in any way
 */
class OGRPolygonAdapter: public OGRPolygonalAdapter{
private:
    OGRPolygonAdapter(OGRUniquePtr<OGRPolygon> && polygonPtr, Canonical canonical)
        :OGRPolygonalAdapter(std::move(polygonPtr), canonical) {}

    /**
     * @brief Adopt a polygon which is known to be in canonical form already
     *
     * The members of a canonical multi-polygon are canonical themselves, so handing one out does
     * not have to pay for another GEOS normalisation. Reserved for OGRMultiPolygonAdapter, as
     * adopting a polygon which is not actually canonical breaks the invariant operator== and
     * hash() rely on.
     */
    static OGRPolygonAdapter fromCanonicalPolygon(const OGRPolygon & polygon) {
        return OGRPolygonAdapter(OGRUniquePtr<OGRPolygon>(polygon.clone()), Canonical{});
    }

    /**
     * @brief Wrap a polygon owned by somebody else, without copying it
     *
     * Only for operating on the members of a multi-polygon in place: the multi-polygon outlives
     * the loop, so nothing can dangle. Never hand one of these out.
     * @warning the result must not outlive the owner of the polygon
     */
    static OGRPolygonAdapter borrowed(OGRPolygon * polygon) {
        return OGRPolygonAdapter(__impl::borrow(polygon), Canonical{});
    }

    /**
     * @brief The boundary of this polygon, without copying its points
     * @warning only for use within an operation, which this polygon outlives
     */
    OGRRingAdapter borrowedBoundary() const {
        return OGRRingAdapter::borrowed(geomPtr->getExteriorRing());
    }

    /**
     * @brief The holes of this polygon, without copying their points
     * @note these are wound against the shell and therefore not in canonical form: their geometry
     * is exact, but they must not be compared or hashed. getHoles() hands out canonical copies.
     * @warning only for use within an operation, which this polygon outlives
     */
    auto borrowedHoles() const -> fishnet::util::forward_range_of<OGRRingAdapter> auto {
        return std::ranges::views::iota(0, geomPtr->getNumInteriorRings())
            | std::ranges::views::transform([this](int i){
                return OGRRingAdapter::borrowed(geomPtr->getInteriorRing(i));
            });
    }

    friend class OGRMultiPolygonAdapter;
    friend class OGRGeometryAdapter;

    /**
     * @brief Hand the wrapped polygon over as a standalone geometry, without copying it
     * @note the polygon is left moved-from and must not be used again
     */
    OGRUniquePtr<OGRGeometry> releaseGeometry() && noexcept {
        auto deleter = geomPtr.get_deleter();
        return OGRUniquePtr<OGRGeometry>(geomPtr.release(), deleter);
    }

public:
    using OGRPolygonalAdapter::contains;
    using numeric_type = double;
    static constexpr GeometryType type = GeometryType::POLYGON;

    OGRPolygonAdapter(OGRUniquePtr<OGRPolygon> && polygonPtr):OGRPolygonalAdapter(std::move(polygonPtr)) {}

    OGRPolygonAdapter(IPolygon auto const & polygon):OGRPolygonalAdapter(toOGRPolygon(polygon)) {}

    /**
     * @note the wrapped polygon is canonical, hence so is its exterior ring
     */
    OGRRingAdapter getBoundary() const {
        return OGRRingAdapter::fromCanonicalRing(*exteriorRing());
    }

    /**
     * @note the wrapped polygon is canonical, so its holes only have to be turned the right way
     * round, @see OGRRingAdapter::fromCanonicalHole
     */
    std::vector<OGRRingAdapter> getHoles() const {
        std::vector<OGRRingAdapter> holes;
        holes.reserve(static_cast<size_t>(geomPtr->getNumInteriorRings()));
        for(int i = 0; i < geomPtr->getNumInteriorRings(); ++i){
            holes.emplace_back(OGRRingAdapter::fromCanonicalHole(*geomPtr->getInteriorRing(i)));
        }
        return holes;
    }

    bool isSimple() const {
        return geomPtr->getNumInteriorRings() == 0;
    }

    /**
     * @brief Copy the polygon out into a fishnet Polygon, which precomputes its segments
     * @see OGRRingAdapter::toNative()
     */
    Polygon<double> toNative() const {
        std::vector<Ring<double>> holes;
        holes.reserve(static_cast<size_t>(geomPtr->getNumInteriorRings()));
        for(const auto & hole : getHoles()){
            holes.push_back(hole.toNative());
        }
        return Polygon<double>(getBoundary().toNative(), holes, true);
    }

    /**
     * @brief Area weighted centroid of the polygon, obtained by decomposition into its rings
     * @note fishnet defines the centroid of a ring as the mean of its points, which is what the
     * decomposition below is built on; it is not the geometric centroid OGR/GEOS would compute.
     * The rings are weighted by their share of the area, as Vec2D rounds coordinates below epsilon
     * to zero, which would round away centroid * area for slivers (see Polygon::centroid).
     */
    Vec2DReal centroid() const {
        auto boundary = borrowedBoundary();
        auto accumulatedArea = boundary.area();
        for(const auto & hole : borrowedHoles()){
            accumulatedArea -= hole.area();
        }
        auto accumulatedCentroid = boundary.centroid() * (boundary.area() / accumulatedArea);
        for(const auto & hole : borrowedHoles()){
            accumulatedCentroid = accumulatedCentroid + hole.centroid() * (-hole.area() / accumulatedArea);
        }
        return accumulatedCentroid;
    }

    bool containsInHole(const IPoint auto & point) const {
        return std::ranges::any_of(borrowedHoles(), [&point](const auto & hole){return hole.contains(point);});
    }

    bool containsInHole(const IPolygon auto & other) const {
        auto otherBoundary = other.getBoundary();
        return std::ranges::any_of(borrowedHoles(), [&otherBoundary](const auto & hole){return hole.contains(otherBoundary);});
    }

    bool intersects(const LinearGeometry auto & linearFeature) const {
        if(borrowedBoundary().intersects(linearFeature))
            return true;
        return std::ranges::any_of(borrowedHoles(), [&linearFeature](const auto & hole){return hole.intersects(linearFeature);});
    }

    std::unordered_set<Vec2DReal> intersections(const LinearGeometry auto & linearFeature) const {
        std::unordered_set<Vec2DReal> intersectionSet;
        for(const auto & point : borrowedBoundary().intersections(linearFeature)){
            intersectionSet.insert(point);
        }
        for(const auto & hole : borrowedHoles()){
            for(const auto & point : hole.intersections(linearFeature)){
                intersectionSet.insert(point);
            }
        }
        return intersectionSet;
    }

    bool crosses(const IPolygon auto & other) const {
        if(not this->aaBB().overlap(other.aaBB()))
            return false; // polygons whose bounding boxes are apart cannot share a single point
        if(containsInHole(other))
            return false;
        auto otherBoundary = other.getBoundary();
        if(borrowedBoundary().crosses(otherBoundary))
            return true;
        return std::ranges::any_of(borrowedHoles(), [&otherBoundary](const auto & hole){return hole.crosses(otherBoundary);});
    }

    bool touches(const IPolygon auto & other) const {
        if(not this->aaBB().overlap(other.aaBB()))
            return false; // polygons whose bounding boxes are apart cannot touch
        if(this->crosses(other))
            return false;
        auto otherBoundary = other.getBoundary();
        if(borrowedBoundary().touches(otherBoundary))
            return true;
        return std::ranges::any_of(borrowedHoles(), [&otherBoundary](const auto & hole){
            // a hole fully contains the other polygon and touches it at least at one point
            return hole.contains(otherBoundary) && std::ranges::any_of(otherBoundary.getPoints(),[&hole](const auto & p){
                return hole.isOnBoundary(p);
            });
        });
    }

    /**
     * @brief Distance between the two polygons
     * @return 0 if the polygons overlap in any way, i.e. if one contains or touches the other,
     * otherwise the distance between their boundaries, measured through a hole where applicable
     */
    double distance(const IPolygon auto & other) const {
        // polygons whose bounding boxes are apart can neither contain nor touch each other, so
        // the gap between their boundaries is all there is left to compute
        if(this->aaBB().overlap(other.aaBB())){
            if(this->contains(other))
                return 0;
            if(this->touches(other))
                return 0;
        }
        auto otherBoundary = other.getBoundary();
        for(const auto & hole : borrowedHoles()){
            if(hole.contains(otherBoundary)){
                return shapeDistance(hole, otherBoundary);
            }
        }
        return borrowedBoundary().distance(otherBoundary);
    }

    /**
     * @brief Distance between two OGR backed polygons
     * @note geomPtr already has its holes subtracted, so GEOS' own Distance() returns 0 exactly
     * when the two regions overlap or touch and the true gap to the nearest boundary (a hole's
     * rim included) otherwise - the same result the generic overload above computes by hand,
     * without having to inspect bounding boxes or holes here
     */
    double distance(const OGRPolygonAdapter & other) const {
        return geomPtr->Distance(other.geomPtr.get());
    }
};
static_assert(IPolygon<OGRPolygonAdapter>);
static_assert(Shape<OGRPolygonAdapter>);
}
