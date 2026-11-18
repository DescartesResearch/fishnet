#pragma once
#include <algorithm>
#include <fishnet/Concepts.hpp>
#include <fishnet/Constants.hpp>
#include <fishnet/Degrees.hpp>
#include <fishnet/CantorPairing.hpp>
#include <fishnet/IPoint.hpp>

namespace fishnet::geometry{

/**
 * @brief Implementation of a two-dimensional vector
 * 
 * @tparam T numeric type used for computations
 */
template<fishnet::math::Number T=fishnet::math::DEFAULT_NUMERIC>
class Vec2D {
private: 
    constexpr static auto hasher = std::hash<fishnet::math::DEFAULT_NUMERIC>{}; //convert all to double to keep hash consistent with equality function
public:   
    T x;
    T y;

    using numeric_type = T;
    constexpr static GeometryType type = GeometryType::POINT;

    /**
     * @brief Helper factory method to construct the correct type of vector depending on the numeric types
     * 
     * @tparam U numeric type for x
     * @tparam V numeric type for y
     * @param x 
     * @param y 
     * @return vector of a common numeric type that avoid narrowing conversions
     */
    template<fishnet::math::Number U, fishnet::math::Number V>
    constexpr static auto construct(U x, V y) noexcept{
        if constexpr(std::is_same_v<U,V>){
            return Vec2D<U>(x,y);
        }else if constexpr(std::integral<U> && std::integral<V>){
            if constexpr(sizeof(U) > sizeof(V)){ // use bigger integral type (U=long, V=int -> Vec2D<long>)
                return Vec2D<U>(x,y);
            }else{
                return Vec2D<V>(x,y);
            }
        }else {
            return Vec2D<fishnet::math::DEFAULT_NUMERIC>(x,y); // use default numeric type in any other case
        }
    }

    constexpr Vec2D(T x, T  y):x(x),y(y){
        if (fishnet::math::isZero(x)) // utilize helper function to round floating point values very close to zero.
             this->x= 0.0;
        if (fishnet::math::isZero(y))
             this->y= 0.0;
    }

    /**
     * @brief Constructor for heterogenous types
     * Converts to the default numeric type
     * @tparam U numeric type of x
     * @tparam R numeric type of y
     * @tparam typename _ enable if -> only allow if T is the default numeric type and if either U or R is not the same type as T to prevent ambiguities
     * @tparam std::is_same_v<T,fishnet::math::DEFAULT_NUMERIC>> 
     */
    template<fishnet::math::Number U, fishnet::math::Number R, typename = std::enable_if_t<(!std::is_same_v<U,T> || ! std::is_same_v<R,T>)&& std::is_same_v<T,fishnet::math::DEFAULT_NUMERIC>>>
    constexpr Vec2D(U x, R y):Vec2D(static_cast<T>(x),static_cast<T>(y)){}

    /**
     * @brief Default construction to (0,0)
     * 
     */
    constexpr Vec2D():x(0),y(0){}

    /**
     * @brief Cast operator from Vec2D<T> to Vec2D<U>
     * 
     * @tparam U numeric type of resulting vector
     * @return Vec2D<U> 
     */
    template<fishnet::math::Number U>
    constexpr operator Vec2D<U> () const noexcept{
        return Vec2D<U>(static_cast<U>(x),static_cast<U>(y));
    }

    constexpr T getX() const noexcept{
        return x;
    }

    constexpr T getY() const noexcept{
        return y;
    }

    /**
     * @brief Negation operator
     * e.g.: -Vec(2,1) => Vec(-2,-1)
     * @return constexpr Vec2D<T> 
     */
    constexpr Vec2D<T> operator - () const {
        return Vec2D<T>(-x,-y);
    }

    constexpr auto operator+(const IPoint auto & other) const noexcept{
        return construct(x + other.getX(), y + other.getY());
    }

    constexpr auto operator-(const IPoint auto & other) const noexcept{
        return construct(x-other.getX(), y-other.getY());
    }

    template<fishnet::math::Number U>
    constexpr auto operator*(U scalar) const noexcept{
        return construct(x*scalar,y*scalar);
    }

    constexpr auto operator/(fishnet::math::DEFAULT_NUMERIC scalar) const noexcept{
        if(scalar==0) 
            scalar=1; // prevent division by zero, return the same Vec2D instead
        return construct(static_cast<fishnet::math::DEFAULT_NUMERIC>(x)/scalar,static_cast<fishnet::math::DEFAULT_NUMERIC>(y)/scalar);
    }

    template<fishnet::math::Number U>
    constexpr bool operator==(const Vec2D<U> & other) const noexcept{
        return fishnet::math::areEqual(x,other.x) and fishnet::math::areEqual(y,other.y);
    }

    constexpr auto dot(const IPoint auto & other) const noexcept{
        return x * other.getX() + y * other.getY();
    }

    constexpr auto cross(const IPoint auto & other) const noexcept{
        return x * other.getY() - y * other.getX();
    }

    /**
     * @brief Parallel if, placed at a common origin, the tip of the longer vector lies within EPSILON
     * of the line through the shorter one, i.e. both geometries deviate by less than EPSILON over their extent.
     *
     * |cross| = |a| * |b| * sin(angle), so a fixed bound on cross depends on both lengths: for lon/lat
     * edges of 1e-5 degrees (about 1 m) |cross| < 1e-12 accepted angles up to 0.6 degrees as parallel.
     * The distance of the longer tip from the other line is |cross| / min(|a|, |b|), so the check is
     *   |cross| <= EPSILON * min(|a|, |b|)   <=>   cross² <= EPSILON² * min(|a|², |b|²)
     * (squared to avoid a sqrt; both sides are non-negative). A zero vector is parallel to every vector.
     */
    constexpr bool isParallel(const IPoint auto & other) const noexcept{
        return isBelowEpsilonTimesMinLength(cross(other), other);
    }

    /**
     * @brief Orthogonal if, placed at a common origin, the tip of the longer vector lies within EPSILON
     * of the line perpendicular to the shorter one (same scaling as isParallel, with dot instead of cross):
     *   |dot| <= EPSILON * min(|a|, |b|)   <=>   dot² <= EPSILON² * min(|a|², |b|²)
     * A zero vector is orthogonal to every vector.
     */
    constexpr bool isOrthogonal(const IPoint auto & other) const noexcept{
        return isBelowEpsilonTimesMinLength(dot(other), other);
    }

private:
    /// product² <= EPSILON² * min(|this|², |other|²), for a cross or dot product of this and other
    constexpr bool isBelowEpsilonTimesMinLength(fishnet::math::Number auto product, const IPoint auto & other) const noexcept{
        using FLOAT_TYPE = fishnet::math::DEFAULT_FLOATING_POINT;
        constexpr FLOAT_TYPE squaredEpsilon = fishnet::math::EPSILON * fishnet::math::EPSILON;
        FLOAT_TYPE squaredOtherLength = FLOAT_TYPE(other.getX()) * FLOAT_TYPE(other.getX()) + FLOAT_TYPE(other.getY()) * FLOAT_TYPE(other.getY());
        return FLOAT_TYPE(product) * FLOAT_TYPE(product) <= squaredEpsilon * std::min(FLOAT_TYPE(dot(*this)), squaredOtherLength);
    }

public:

    constexpr fishnet::math::DEFAULT_FLOATING_POINT length() const{
        return sqrt(static_cast<fishnet::math::DEFAULT_FLOATING_POINT>(dot(*this)));
    }

    constexpr fishnet::math::DEFAULT_FLOATING_POINT distance(const IPoint auto & other) const {
        return (*this-other).length();
    }

    constexpr Vec2D<T> orthogonal() const noexcept{
        return {y,-x};
    }

    constexpr Vec2D<fishnet::math::DEFAULT_NUMERIC> normalize() const{
        return *this / length();
    }

    /**
     * Computes the angle (counterclockwise) for the direction vector with reference at reference point, starting rotation on x-axis
     * @param reference for the angle
     * @return
     */
    fishnet::math::Radians angle(const IPoint auto & reference) const{
        auto dir = *this - reference;
        return fishnet::math::Radians::atan2(dir.y,dir.x);
    }

    /**
     * Calculates the angle with the previous method (starting from x-axis and performing a counterclockwise rotation)
     * then rotates the angle by angleRotate
     * @param reference
     * @param angleRotate
     * @return
     */
    fishnet::math::Radians angle(const IPoint auto & reference, fishnet::math::Radians angleRotate) const{
        return angle(reference) + angleRotate;
    }

    constexpr size_t hash() const noexcept {
        size_t xHash = hasher(this->x);
        size_t yHash = hasher(this->y);
        return fishnet::math::CantorPairing(xHash,yHash); // utilize cantor pairing to keep hashes unique: hash(Vec2D(2,1)) != hash(Vec2D(1,2))
    }

    constexpr std::string toString() const noexcept{
        return "(" + std::to_string(x) + "," + std::to_string(y) + ")";
    }

};

/**
 * @brief Comparator for lexigraphically-ordering of Vec2D objects
 * Compares first the x-Coordinate and then the y-Coordinate
 */
struct LexicographicOrder{
    template<fishnet::math::Number T,fishnet::math::Number U>
    constexpr bool operator()(const Vec2D<T> & lhs, const Vec2D<U> & rhs)const noexcept {
        if(fishnet::math::areEqual(lhs.x,rhs.x))
             return lhs.y < rhs.y;
        return lhs.x < rhs.x;
    }
};

/**
 * @brief Comparator for lexigraphically-ordering of Vec2D objects, with y Coordinate first
 */
struct YLexicographicOrder{
    template<fishnet::math::Number T,fishnet::math::Number U>
    constexpr inline bool operator()(const Vec2D<T> & lhs, const Vec2D<U> & rhs)const noexcept {
        if(fishnet::math::areEqual(lhs.y,rhs.y))
             return lhs.x < rhs.x;
        return lhs.y < rhs.y;
    }
};

static_assert(IPoint<Vec2D<double>>);
static_assert(IPoint<Vec2D<int>>);

using Vec2DStd = Vec2D<fishnet::math::DEFAULT_NUMERIC>;
using Vec2DReal =Vec2D<fishnet::math::DEFAULT_FLOATING_POINT>;

// Explicit template instantiation for the default numeric type
template class fishnet::geometry::Vec2D<fishnet::math::DEFAULT_NUMERIC>;  
}

