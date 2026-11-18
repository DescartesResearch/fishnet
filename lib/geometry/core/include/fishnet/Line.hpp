#pragma once
#include <fishnet/Option.hpp>
#include <fishnet/IGeometry.hpp>
#include <fishnet/LinearIntersection.hpp>
#include "Vec2D.hpp"

namespace fishnet::geometry{

/**
 * @brief Implementation of a line
 * 
 * @tparam T numeric type used for computations
 */
template<fishnet::math::Number T = fishnet::math::DEFAULT_NUMERIC>
class Line{
private:
    Vec2D<T> _p;
    Vec2D<T> _q;
    constexpr static auto hasher = std::hash<fishnet::math::DEFAULT_FLOATING_POINT>{};
    const static inline Line<T> yAxis = Line<T>(Vec2D<T>(0,0),Vec2D<T>(0,1));
public:

    /**
     * @brief Vertical line factory 
     * 
     * @param x x-axis intersection of the vertical line
     * @return constexpr Line<T> 
     */
    constexpr static Line<T> verticalLine(T x) noexcept {
        return Line<T>(Vec2D<T>(x,0),Vec2D<T>(x,1));
    }

    /**
     * @brief Horizontal line factory
     * 
     * @param y y-axis intersection of the horizontal line
     * @return constexpr Line<T> 
     */
    constexpr static Line<T> horizontalLine(T y) noexcept {
        return Line<T>(0,y);
    }

    using numeric_type =  T;
    constexpr static GeometryType type = GeometryType::LINE;

    /**
     * @brief Constructor for a line formed by two points
     * 
     */
    constexpr Line(Vec2D<T> _p , Vec2D<T> _q):_p(_p),_q(_q){
        if (_p == _q) 
            throw std::invalid_argument("Coinciding Points cannot define a Line");
    }

    /**
     * @brief Constructor for a line with heterogenous numeric types
     * Construct new line by converting to the default numeric type
     * @tparam U numeric type != T
     * @tparam typename enable_if -> only allow this constructor if T is the numeric type:
     * @tparam std::is_same_v<T,fishnet::math::DEFAULT_NUMERIC>> 
     */
    template<fishnet::math::Number U, typename = std::enable_if_t<!std::is_same_v<U,T> && std::is_same_v<T,fishnet::math::DEFAULT_NUMERIC>>>
    constexpr Line(Vec2D<T> _p, Vec2D<U> _q):Line(_p,static_cast<Vec2D<T>>(_q)){}

    /**
     * @brief Constructor for a line with heterogenous numeric types
     * Construct new line by converting to the default numeric type
     * @tparam U numeric type != T
     * @tparam typename enable_if -> only allow this constructor if T is the numeric type:
     * @tparam std::is_same_v<T,fishnet::math::DEFAULT_NUMERIC>> 
     */
    template<fishnet::math::Number U, typename = std::enable_if_t<!std::is_same_v<U,T> && std::is_same_v<T,fishnet::math::DEFAULT_NUMERIC>>>
    constexpr Line(Vec2D<U> _p , Vec2D<T> _q):Line(static_cast<Vec2D<T>>(_p),_q){}

    /**
     * @brief Constructor of line using slope and y intercept
     * 
     */
    constexpr Line(T slope, T yIntercept):_p(Vec2D<T>(0,yIntercept)),_q(Vec2D<T>(1,slope+yIntercept)){}

    /**
     * @brief Constructor of line using slope and y intercept with heterogenous type
     * 
     * @tparam U numeric type != T
     * @tparam typename enable_if -> only allow this constructor if T is the numeric type:
     * @tparam std::is_same_v<T,fishnet::math::DEFAULT_NUMERIC>>
     */
    template<fishnet::math::Number U, typename = std::enable_if_t<!std::is_same_v<U,T> && std::is_same_v<T,fishnet::math::DEFAULT_NUMERIC>>>
    constexpr Line(T slope, U yIntercept):Line(slope,fishnet::math::DEFAULT_NUMERIC(yIntercept)){}

    /**
     * @brief Constructor of line using slope and y intercept with heterogenous type
     * 
     * @tparam U numeric type != T
     * @tparam typename enable_if -> only allow this constructor if T is the numeric type:
     * @tparam std::is_same_v<T,fishnet::math::DEFAULT_NUMERIC>>
     */
    template<fishnet::math::Number U, typename = std::enable_if_t<!std::is_same_v<U,T> && std::is_same_v<T,fishnet::math::DEFAULT_NUMERIC>>>
    constexpr Line(U slope, T yIntercept):Line(fishnet::math::DEFAULT_NUMERIC(slope),yIntercept){}

    constexpr Vec2D<T> p() const noexcept{
        return _p;
    }

    constexpr Vec2D<T> q() const noexcept{
        return _q;
    }

    constexpr Line<T> toLine() const noexcept{
        return Line(_p,_q);
    }

    constexpr Vec2D<T> direction() const noexcept{
        return _q-_p;
    }

    constexpr bool isVertical() const noexcept{
        return _p.x == _q.x;
    }

    constexpr bool isHorizontal() const noexcept{
        return _p.y == _q.y;
    }

    constexpr fishnet::Option<fishnet::math::DEFAULT_FLOATING_POINT> yIntercept() const noexcept{
        fishnet::Option<Vec2DReal> intersectionWithY = intersection(yAxis);
        [[likely]] if (intersectionWithY){
            return fishnet::Option(intersectionWithY->y);
        }
        return std::nullopt;
    }

    constexpr fishnet::math::DEFAULT_FLOATING_POINT slope() const noexcept{
        [[unlikely]] if (direction().x == 0) 
            return std::numeric_limits<fishnet::math::DEFAULT_FLOATING_POINT>::max();
        return fishnet::math::DEFAULT_FLOATING_POINT(direction().y) / fishnet::math::DEFAULT_FLOATING_POINT(direction().x);
    }

    constexpr bool contains(IPoint auto const & point) const noexcept{
        if(__impl::PointOnLinePosition(_p, direction(), point).isOnLine())
            return true;
        // Only checked on rejection (hot path): points equal to p or q per Vec2D::operator==, which
        // allows up to EPSILON per coordinate. Also covers a degenerate line through a single point.
        return _p == point or _q == point;
    }

    constexpr bool isLeft(IPoint auto const & point) const noexcept {
        return direction().cross(point-_p) > 0;
    }

    constexpr bool isRight(IPoint auto const & point) const noexcept {
        return direction().cross(point-_p) < 0;
    }

    constexpr bool isParallel(LinearGeometry auto const& other) const noexcept {
        return areParallel(*this,other);
    }

    constexpr bool intersects(LinearGeometry auto const& other) const noexcept{
        return intersect(*this,other);
    }

    /// Equal if parallel and a point of the other line lies on this line
    template<fishnet::math::Number U>
    constexpr bool operator==(const Line<U> & other) const noexcept {
        return this->isParallel(other) and this->contains(other.p());
    }

    constexpr fishnet::Option<Vec2DReal> intersection(LinearGeometry auto const& other) const noexcept {
        return linearIntersection(*this,other);
    }

    constexpr size_t hash() const noexcept {
        size_t slopeHash = hasher(this->slope());
        size_t yInterceptHash = hasher(this->yIntercept().value_or(this->_p.x));
        return fishnet::math::CantorPairing(slopeHash,yInterceptHash);
    }

    constexpr std::string toString() const {
        auto t = yIntercept();
        if (t){
            return "Line y = " + std::to_string(slope())+" * x + "+std::to_string(t.value());
        }
        return "Vertical Line x = "+std::to_string(_p.x);
    }
};

//Deduction guides
template<math::Number T>
Line(Vec2D<T>,Vec2D<T>) -> Line<T>;

const static inline Line<double> X_AXIS = Line<double>(0.0,0.0);
const static inline Line<double> Y_AXIS = Line<double>(Vec2D<double>(0,0),Vec2D<double>(0,1));
static_assert(ILine<Line<double>>);
static_assert(LinearGeometry<Line<double>>);

// Explicit template instantiation
template class Line<fishnet::math::DEFAULT_NUMERIC>;
}
