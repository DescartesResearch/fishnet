#pragma once
#include <fishnet/NumericConcepts.hpp>
#include <cmath>

namespace fishnet::math{
    constexpr static double PI = 3.14159265358979323846;
    constexpr static double TWO_PI = 2*PI; 
    constexpr static double DEG_TO_RAD = PI / 180.0;
    constexpr static double RAD_TO_DEG = 180.0 / PI;
    constexpr static double DOUBLE_EPSILON = 1e-12;
    constexpr static float FLOAT_EPSILON=1e-6f;

    template<std::floating_point F>
    constexpr static DEFAULT_FLOATING_POINT getDefaultEpsilon(){
        if constexpr(std::same_as<DEFAULT_FLOATING_POINT,double>) return DOUBLE_EPSILON;
        if constexpr(std::same_as<DEFAULT_FLOATING_POINT,float>) return FLOAT_EPSILON;
        else return std::numeric_limits<F>::epsilon();
    }

    /**
     * @brief Tolerance for geometric comparisons: two coordinates closer than EPSILON are equal.
     *
     * EPSILON is a coordinate distance, so only compare it with quantities that are lengths:
     * coordinate differences, a perpendicular distance, a distance along a line.
     * - Products of vectors are not lengths: cross(a, b) and dot(a, b) are length², and they shrink
     *   with the vector lengths. Compare them with EPSILON * length, e.g. cross² <= EPSILON² * |a|².
     * - Ratios are not lengths: a position along a segment normalised to [0,1] or a sine. Multiply
     *   them by the length they were divided by first.
     * Comparing those directly with EPSILON breaks silently on short geometries: in lon/lat a
     * building edge is about 1e-4 degrees, so products and normalised values reach the size of
     * EPSILON or of rounding errors although nothing is degenerate.
     * See Vec2D::isParallel and __impl::PointOnLinePosition (LinearIntersection.hpp) for examples.
     */
    constexpr static DEFAULT_FLOATING_POINT EPSILON = getDefaultEpsilon<DEFAULT_FLOATING_POINT>();

    template<Number T>
    constexpr inline bool isZero(T number) noexcept{
        if constexpr(std::integral<T>) return number == 0;
        if constexpr(std::same_as<double,T>) return fabs(number) < DOUBLE_EPSILON;
        if constexpr(std::same_as<float,T>) return fabs(number) <FLOAT_EPSILON;
        return number==0; //default case
    }

    template<Number T, Number U>
    constexpr inline bool areEqual(T lhs, U rhs) noexcept {
        if constexpr(std::integral<T> && std::integral<U>) return lhs==rhs;
        if constexpr(std::integral<T> && std::floating_point<U>) return isZero(U(lhs)-rhs);
        if constexpr(std::integral<U> && std::floating_point<T>) return isZero(T(rhs)-lhs);
        if constexpr(std::floating_point<T> && std::floating_point<U>) return isZero(lhs-rhs);
    }
}