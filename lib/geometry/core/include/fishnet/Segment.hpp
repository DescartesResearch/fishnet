#pragma once
#include <array>
#include <algorithm>
#include "Vec2D.hpp"
#include "Line.hpp"

namespace fishnet::geometry{
/**
 * @brief Implementation of a segment
 * 
 * @tparam T numeric type used for computations
 */
template<fishnet::math::Number T = fishnet::math::DEFAULT_NUMERIC>
class Segment{
private:
    Vec2D<T> _p;
    Vec2D<T> _q;
public:

    using numeric_type = T;
    constexpr static GeometryType type = GeometryType::SEGMENT;

    constexpr Segment(Vec2D<T> p, Vec2D<T> q):_p(p),_q (q){}

    template<fishnet::math::Number U, typename = std::enable_if_t<!std::is_same_v<U,T> && std::is_same_v<T,fishnet::math::DEFAULT_NUMERIC>>>
    constexpr Segment(Vec2D<T> p, Vec2D<U> q):Segment(p,Vec2D<T> (q.x ,q.y)){}

    template<fishnet::math::Number U, typename = std::enable_if_t<!std::is_same_v<U,T> && std::is_same_v<T,fishnet::math::DEFAULT_NUMERIC>>>
    constexpr Segment(Vec2D<U> p, Vec2D<T> q):Segment(Vec2D<T>(p.x ,p.y) ,q){}

    constexpr const Vec2D<T> & p() const noexcept {
        return _p;
    }

    constexpr const Vec2D<T> & q() const noexcept {
        return _q;
    }

    constexpr Segment<T> flip() const noexcept {
        return Segment(this->_q,this->_p);
    }

    constexpr Vec2D<T> direction() const noexcept {
        return _q-_p;
    }

    constexpr const Vec2D<T> & upperEndpoint() const noexcept {
        if (fishnet::math::areEqual(_p.y,_q.y)) 
            return _p.x > _q.x ? _p:_q;
        return _p.y > _q.y ? _p:_q;
    }

    constexpr const Vec2D<T> & lowerEndpoint() const noexcept {
        if(fishnet::math::areEqual(_p.y,_q.y)) 
            return _p.x < _q.x ? _p:_q;
        return _p.y < _q.y ? _p:_q;
    }

    constexpr const Vec2D<T> & leftEndpoint() const noexcept {
        if(fishnet::math::areEqual(_p.x,_q.x))
            return _p.y < _q.y ? _p : _q;
        return _p.x < _q.x ? _p : _q;
    }

    constexpr const Vec2D<T> & rightEndpoint() const noexcept {
        if(fishnet::math::areEqual(_p.x,_q.x))
            return _p.y > _q.y ? _p : _q;
        return _p.x > _q.x ? _p : _q;
    }

    constexpr bool isEndpoint(IPoint auto const & point) const noexcept {
        return _p == point or _q == point;
    }

    constexpr fishnet::math::DEFAULT_FLOATING_POINT length() const noexcept {
        return direction().length();
    }

    constexpr bool isValid() const noexcept{
        return length() > 0;
    }

    constexpr Line<T> toLine() const noexcept {
        return Line<T>(_p,_q);
    }

    constexpr bool isParallel(LinearGeometry auto const& other)const noexcept{
        return areParallel(*this,other);
    }

    constexpr bool intersects(LinearGeometry auto const& other)const noexcept{
        return intersect(*this,other);
    }

    /**
     * @brief Two Segments have an Overlay if the are contained on the same line: 
     *  -> the Segments must be parallel and at least share on point.
     * 
     * @tparam U 
     * @param other 
     * @return true 
     * @return false 
     */
    constexpr bool overlaps(ISegment auto const & other) const noexcept{
        return isParallel(other) and (contains(other.p()) or contains(other.q()));
    }

    constexpr bool contains(ISegment auto const & other) const noexcept {
        return isParallel(other) and contains(other.p()) and contains(other.q());
    }

    /**
     * @brief Two Segments touch each other when one endpoint is equal and
     * neither Segment is fully contained in the other one
     * 
     * @tparam U 
     * @param other 
     * @return true 
     * @return false 
     */
    constexpr bool touches(ISegment auto const & other) const noexcept{
        return (this->_p == other.p() or this->_p == other.q() or this->_q == other.q() or this->_q == other.p())
            && not contains(other)
            && not other.contains(*this);
    }

    template<fishnet::math::Number U>
    constexpr bool operator==(const Segment<U> & other) const noexcept {
        return (this->_p == other.p() and this->_q == other.q()) or (this->_q==other.p() and this->_p == other.q());
    }

    constexpr bool contains(IPoint auto const & point)const noexcept{
        // on the line through p and q, and its projection between p and q (see Line::contains)
        auto position = __impl::PointOnLinePosition(_p, direction(), point);
        if (position.isOnLine() and position.isBetweenEnds())
            return true;
        // Only checked on rejection (hot path): points equal to p or q per Vec2D::operator==, which
        // allows up to EPSILON per coordinate. Also covers the 0-length segment (isOnLine is false).
        return _p == point or _q == point;
    }

    constexpr fishnet::math::DEFAULT_FLOATING_POINT distance(IPoint auto const & point) const noexcept {
        auto orthogonalDirection = this->direction().orthogonal();
        auto orthogonalLine = Line(point, point + orthogonalDirection);
        auto intersection = this->intersection(orthogonalLine);
        if(intersection) 
            return point.distance(*intersection); // intersection on the segment of orthogonal line between segment and point
        return std::min(_p.distance(point), _q.distance(point)); // return closest endpoint of segment to the point otherwise
    }

    constexpr fishnet::math::DEFAULT_FLOATING_POINT distance(ISegment auto const & other) const noexcept {
        std::array<fishnet::math::DEFAULT_FLOATING_POINT,4> candidates {
            other.distance(this->_p),
            other.distance(this->_q),
            this->distance(other.p()),
            this->distance(other.q())
        };
        return *std::ranges::min_element(candidates);
    }

    constexpr fishnet::Option<Vec2DReal> intersection(LinearGeometry auto const& other)const noexcept {
        return linearIntersection(*this,other);
    }

    constexpr size_t hash() const noexcept {
        return this->_p.hash() ^ this->_q.hash();
    }
    
    constexpr std::string toString() const noexcept {
        return "[" + this->_p.toString() +","+this->_q.toString()+"]";
    }
};
static_assert(ISegment<Segment<double>>);
static_assert(LinearGeometry<Segment<double>>);
// Explicit template instantiation
template class Segment<fishnet::math::DEFAULT_NUMERIC>;
} // namespace fishnet::geometry