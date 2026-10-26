#pragma once
#include <fishnet/IGeometry.hpp>
#include "Ring.hpp"
#include "SimplePolygon.hpp"


namespace fishnet::geometry {
/**
 * @brief Polygon implementation
 * A Polygon consist of a boundary (IRing) and zero or more holes (IRing)
 * @tparam T numeric_type used for computations
 */
template<fishnet::math::Number T>
class Polygon : public SimplePolygon<T>{
public:
    using SimplePolygon<T>::contains;
private:
    std::vector<Ring<T>> holes;

    /**
     * @brief Helper method to adapt point location queries to holes
     * Points within a hole are considered outside of the polygon
     * @param point 
     * @return PointLocation of the point (INSIDE | BOUNDARY | OUTSIDE) 
     */
    constexpr PointLocation polygonPointLocation(IPoint auto const & point) const noexcept {
        PointLocation ofBoundary = this->getPointLocation(point);
        if (ofBoundary != PointLocation::INSIDE) {
            return ofBoundary;
        }
        if (std::ranges::any_of(getHoles(),[point](const auto & hole){
            return hole.isInside(point);
        })) return PointLocation::OUTSIDE;
        return PointLocation::INSIDE;

    }

    constexpr static std::vector<Ring<T>> copyRings(util::forward_range_of<Ring<T>> auto const & ringRange) noexcept {
        std::vector<Ring<T>> rings;
        std::ranges::copy(ringRange, std::back_inserter(rings));
        return rings;
    }

    constexpr bool equals(IPolygon auto const & other) const noexcept {
        return this->getBoundary() == other.getBoundary() && std::ranges::is_permutation(this->getHoles(),other.getHoles());
    }

public:
    using numeric_type = T;
    constexpr static GeometryType type = GeometryType::POLYGON;

    /**
     * @brief Construct a new Polygon object using boundary and holes
     * 
     * @param boundary 
     * @param holes 
     */
    Polygon(const Ring<T> & boundary, const std::vector<Ring<T>> & holes = {}, bool checked = false):SimplePolygon<T>(boundary),holes(holes){
        if(checked)
            return;
        if (std::ranges::any_of(holes, [&boundary](const auto & hole){return not boundary.contains(hole);}))
            throw InvalidGeometryException("Hole not contained within Boundary of Polygon");
        if (std::ranges::any_of(holes, [&boundary,&holes](const auto & h1){
            return std::ranges::any_of(holes, [&boundary,&h1](const auto & h2){
                return h1.crosses(h2);
            });
        })) throw InvalidGeometryException("Holes of Polygon are intersecting each other");
    };

    Polygon(const Ring<T> & boundary, const util::forward_range_of<Ring<T>> auto & holes, bool checked = false):Polygon(boundary,std::move(copyRings(holes)), checked) {}

    Polygon(const SimplePolygon<T> & boundary, const std::vector<Ring<T>> & holes = {}, bool checked = false):Polygon(boundary.getBoundary(),holes, checked){}

    Polygon(const SimplePolygon<T> & boundary, const util::forward_range_of<Ring<T>> auto & holes, bool checked = false):Polygon(boundary.getBoundary(),std::move(copyRings(holes)), checked) {}

    constexpr const Ring<T> & getBoundary() const noexcept{
        return static_cast<const Ring<T> &>(*this);
    }

    constexpr util::view_of<Ring<T>> auto getHoles() const noexcept {
        return std::views::all(holes);
    }

    constexpr util::view_of<Segment<T>> auto getSegments() = delete;

    constexpr util::random_access_range_of<Vec2D<T>> auto getPoints() = delete;


    constexpr bool isSimple() const noexcept {
        return holes.size() == 0;
    }

    constexpr const SimplePolygon<T> & toSimple() const noexcept {
        return static_cast<const SimplePolygon<T> & >(*this);
    }

    /**
     * @brief Calculate the area of the polygon
     * Subtract accumulated area of holes from the area contained within the boundary
     * @return constexpr fishnet::math::DEFAULT_FLOATING_POINT 
     */
    constexpr fishnet::math::DEFAULT_FLOATING_POINT area() const noexcept {
        fishnet::math::DEFAULT_FLOATING_POINT area = this->getBoundary().area();
        for(const auto & hole : holes) {
            area -= hole.area();
        }
        return area;
    }

    /**
     * @brief Calculate weighted centroid of the polygon
     * https://en.wikipedia.org/wiki/Centroid
     * @note the rings are weighted by their share of the area rather than by their absolute area,
     * as Vec2D rounds coordinates below epsilon to zero: centroid * area would be rounded away for
     * polygons with an area close to epsilon, e.g. slivers in degree based coordinate systems.
     * @return constexpr Vec2DReal
     */
    constexpr Vec2DReal centroid() const noexcept {
        auto polygonArea = this->area();
        auto accCentroid = this->getBoundary().centroid() * (this->getBoundary().area() / polygonArea);
        for(const auto & hole: this->getHoles()){
            accCentroid = accCentroid + hole.centroid() * (-hole.area() / polygonArea);
        }
        return accCentroid; // -> weighted centroid by decomposition
    }

    constexpr bool inline contains(IPoint auto const & point) const noexcept {
        auto location = polygonPointLocation(point);
        return location == PointLocation::INSIDE || location == PointLocation::BOUNDARY;
    }

    constexpr bool inline contains(ISegment auto const & segment) const noexcept {
        return this->getBoundary().contains(segment) && std::ranges::none_of(this->getHoles(),[&segment](const auto & hole){return hole.intersects(segment);});
    }

    constexpr bool inline isInside(IPoint auto const & point) const noexcept {
        return polygonPointLocation(point) == PointLocation::INSIDE;
    }

    constexpr bool inline isOnBoundary(IPoint auto const & point) const noexcept {
        return polygonPointLocation(point) == PointLocation::BOUNDARY;
    }

    constexpr bool inline isOutside(IPoint auto const & point) const noexcept {
        return polygonPointLocation(point) == PointLocation::OUTSIDE;
    }

    constexpr bool intersects(LinearGeometry auto const & linearFeature) const noexcept {
        return this->getBoundary().intersects(linearFeature) || std::ranges::any_of(getHoles(),[linearFeature](const auto & hole){
            return hole.intersects(linearFeature);
        });
    }

    constexpr std::unordered_set<Vec2D<double>> intersections(LinearGeometry auto const & linearFeature) const noexcept {
        std::unordered_set<Vec2D<double>> intersectionSet;
        std::ranges::for_each(this->getBoundary().intersections(linearFeature),[&intersectionSet](const auto & p){intersectionSet.insert(p);});
        std::ranges::for_each(this->getHoles(), [&intersectionSet,&linearFeature](const auto & hole){
            std::ranges::for_each(hole.intersections(linearFeature),[&intersectionSet](const auto & p){intersectionSet.insert(p);});
        });
        return intersectionSet;
    }

    constexpr bool containsInHole(IPoint auto const & point) const noexcept {
        return std::ranges::any_of(this->getHoles(),[&point](const auto & hole){
            return hole.contains(point);
        });
    }

    constexpr bool containsInHole(IPolygon auto const & other) const noexcept {
        return std::ranges::any_of(this->getHoles(),[&other](const auto & hole){
            return hole.contains(other.getBoundary());
        });
    }

    constexpr bool contains(IPolygon auto const & other) const noexcept {
        if(this->equals(other))
            return true;
        if(containsInHole(other))
            return false; // inside of hole
        return this->getBoundary().contains(other.getBoundary()) && std::ranges::none_of(this->getHoles(),[&other](const auto & hole){
            // the boundary of the other polygon must not be intersected by any hole and the other polygon is not allowed to contain any hole 
            return hole.crosses(other.getBoundary()) || other.getBoundary().contains(hole); 
        });
    }

    constexpr bool crosses(IPolygon auto const & other) const noexcept {
        if(containsInHole(other))
            return false;
        return this->getBoundary().crosses(other.getBoundary()) || std::ranges::any_of(this->getHoles(),[&other](const auto & hole){
            return hole.crosses(other.getBoundary());
        });
    }

    constexpr bool touches(IPolygon auto const & other) const noexcept {
        if(this->crosses(other)) 
            return false;
        return this->getBoundary().touches(other.getBoundary()) || std::ranges::any_of(this->getHoles(), [&other](const auto & hole){
            return hole.contains(other.getBoundary()) && std::ranges::any_of(other.getBoundary().getPoints(),[&hole](const auto & p){
                return hole.isOnBoundary(p); // hole fully contains other polygon and touches it at least at one point
            });
        });
    }

    /**
     * @brief Distance between the two polygons
     * @return 0 if the polygons overlap in any way, i.e. if one contains or touches the other,
     * otherwise the distance between their boundaries, measured through a hole where applicable
     */
    constexpr fishnet::math::DEFAULT_FLOATING_POINT distance(IPolygon auto const & other) const noexcept {
        if(this->contains(other)) return 0;
        if(this->touches(other)) return 0;
        // check if in any hole and compute distance
        for(const auto & hole: getHoles()){
            if(hole.contains(other.getBoundary())){
                return shapeDistance(hole,other.getBoundary());
            }
        }
        // compute boundary distance otherwise
        return this->getBoundary().distance(other.getBoundary());
    }

    template<fishnet::math::Number U>
    constexpr bool operator==(const Polygon<U> & other) const noexcept {
        return this->equals(other);
    }

    constexpr size_t hash() const noexcept{
        auto boundaryHash = this->getBoundary().hash();
        auto holesHashView = this->getHoles() | std::views::transform([](const auto & ring){return ring.hash();});
        auto holesHash = std::accumulate(std::ranges::begin(holesHashView),std::ranges::end(holesHashView),size_t(0));
        return boundaryHash ^ holesHash;
    }

    constexpr std::string toString() const noexcept {
        std::ostringstream oss;
        oss << "Boundary: ";
        oss << this->getBoundary().toString();
        oss << "\nHoles: {";
        bool first = true;
        for(const auto & hole : this->getHoles()){
            if(!first) oss << "},{";
            oss << hole.toString();
            first = false;
        }
        oss << "}";
        return oss.str();
    }
}; 

//Deduction guides
template<math::Number T>
Polygon(const Ring<T> &, auto) -> Polygon<T>;

template<math::Number T>
Polygon(const SimplePolygon<T> &, auto) -> Polygon<T>;

static_assert(IPolygon<Polygon<double>>);
static_assert(Shape<Polygon<double>>);
// Explicit template instantiation
template class Polygon<fishnet::math::DEFAULT_NUMERIC>;
} // namespace fishnet::geometry