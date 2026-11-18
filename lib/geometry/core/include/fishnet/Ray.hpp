#pragma once
#include "Vec2D.hpp"
#include "Line.hpp"

namespace fishnet::geometry{

/**
 * @brief Implementation of a ray
 * A ray consist of an origin and the direction of the ray
 * @tparam T numeric type used for computations
 */
template<fishnet::math::Number T>
class Ray{
private:
    const Vec2D<T> originPoint; 
    const Vec2D<T> directionVector;
public: 
    using numeric_type = T;
    constexpr static GeometryType type = GeometryType::RAY;

    constexpr static Ray<T> up(Vec2D<T> origin) noexcept {
        return Ray<T>(origin, Vec2D<T>(0,1));
    }

    constexpr static Ray<T> down(Vec2D<T> origin) noexcept {
        return Ray<T>(origin, Vec2D<T>(0,-1));
    }

    constexpr static Ray<T> right(Vec2D<T> origin) noexcept {
        return Ray<T>(origin,Vec2D<T>(1,0));
    }

    constexpr static Ray<T> left(Vec2D<T> origin) noexcept {
        return Ray<T>(origin,Vec2D<T>(-1,0));
    }

    constexpr Ray(Vec2D<T> origin, Vec2D<T> direction): originPoint(origin), directionVector(direction){
        [[unlikely]] if (direction == Vec2D(0,0))
             throw std::invalid_argument("Zero Vector not valid for the direction of a Ray");
    }

    template<fishnet::math::Number U, typename = std::enable_if_t<!std::is_same_v<U,T> && std::is_same_v<T,fishnet::math::DEFAULT_NUMERIC>>>
    constexpr Ray(Vec2D<T> origin, Vec2D<U> direction): Ray(origin,static_cast<Vec2D<T>>(direction)){}

    template<fishnet::math::Number U, typename =std::enable_if_t<!std::is_same_v<U,T> && std::is_same_v<T,fishnet::math::DEFAULT_NUMERIC>>>
    constexpr Ray(Vec2D<U> origin ,Vec2D<T> direction): Ray(static_cast<Vec2D<T>>(origin),direction){}

    constexpr Vec2D<T> direction() const noexcept {
        return this->directionVector;
    }    

    constexpr Vec2D<T> origin() const noexcept {
        return this->originPoint;
    }

    constexpr Line<T> toLine() const noexcept {
        return Line<T>(originPoint,originPoint+directionVector);
    }

    constexpr Ray<T> oppositeRay() const noexcept{
        return Ray(originPoint, -directionVector);
    }

    constexpr Ray<T> operator - () const {
        return oppositeRay();
    }

    constexpr bool isParallel(LinearGeometry auto const& other) const noexcept{
        return areParallel(*this,other);
    }

    constexpr bool intersects(LinearGeometry auto const& other) const noexcept {
        return intersect(*this,other);
    }

    constexpr bool contains(IPoint auto const & point) const noexcept{
        // on the line through the origin, and its projection not behind the origin (see Line::contains)
        auto position = __impl::PointOnLinePosition(originPoint, directionVector, point);
        if(position.isOnLine() and position.isNotBeforeStart())
            return true;
        // Only checked on rejection (hot path): points equal to the origin per Vec2D::operator==,
        // which allows up to EPSILON per coordinate.
        return point == originPoint;
    }

    template<fishnet::math::Number U> 
    constexpr bool operator==(const Ray<U> & other) const noexcept {
        return this->originPoint == other.origin() and this->direction().normalize() == other.direction().normalize();
    }

    constexpr std::optional<Vec2DReal> intersection(LinearGeometry auto const& other) const noexcept {
        return linearIntersection(*this,other);
    }

    constexpr size_t hash() const noexcept {
        return fishnet::math::CantorPairing(originPoint.hash(), directionVector.normalize().hash());
    }

    constexpr std::string toString() const {
        return "Ray "+this->originPoint.toString()+" + k * "+this->directionVector.toString();
    }
};

// Explicit template instantiation for the default numeric type
template class Ray<fishnet::math::DEFAULT_NUMERIC>;
static_assert(IRay<Ray<double>>);
static_assert(LinearGeometry<Ray<double>>);
}
