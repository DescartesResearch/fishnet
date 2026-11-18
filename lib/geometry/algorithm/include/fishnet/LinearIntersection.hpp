#pragma once
#include <fishnet/Vec2D.hpp>
#include <fishnet/Option.hpp>
#include <fishnet/IGeometry.hpp>

namespace fishnet::geometry{

namespace __impl {
/**
 * @brief Position of a point relative to the line through p with direction dir, shared by
 * Line::contains, Segment::contains and Ray::contains.
 *
 * Tolerances are EPSILON in coordinate space, so they do not depend on the length of dir.
 * This matters for short directions in lon/lat (e.g. a building edge of 1e-4 degrees): a fixed
 * tolerance on normalised values (lambda in [0,1]) shrinks with the length of dir and falls below
 * the rounding error of coordinates around 39 degrees (about 7e-15).
 *
 * With v = point - p, the two products measure distances scaled by |dir|:
 *   cross(v, dir) = perpendicular distance of point to the line * |dir|
 *   dot(v, dir)   = distance of the projection from p, along the line, * |dir|
 * Dividing by |dir| would need a sqrt (Segment::contains is on a hot path), so every check is
 * multiplied by |dir| instead and then squared, which only needs |dir|² = dot(dir, dir).
 * Squaring keeps the order of a comparison only if both sides are non-negative, see isBetweenEnds.
 */
struct PointOnLinePosition {
    using FLOAT_TYPE = fishnet::math::DEFAULT_FLOATING_POINT;
    constexpr static FLOAT_TYPE squaredEpsilon = fishnet::math::EPSILON * fishnet::math::EPSILON;
    FLOAT_TYPE cross;         // perpendicular distance * |dir|
    FLOAT_TYPE dot;           // distance along the line from p * |dir|
    FLOAT_TYPE squaredLength; // |dir|²

    template<fishnet::math::Number T>
    constexpr PointOnLinePosition(IPoint auto const & p, Vec2D<T> const & dir, IPoint auto const & point) noexcept {
        Vec2DReal toPoint {FLOAT_TYPE(point.x - p.x), FLOAT_TYPE(point.y - p.y)};
        cross = FLOAT_TYPE(toPoint.cross(dir));
        dot = FLOAT_TYPE(toPoint.dot(dir));
        squaredLength = FLOAT_TYPE(dir.dot(dir));
    }

    /**
     * @brief Perpendicular distance below EPSILON:
     *   |cross| / |dir| < EPSILON   <=>   |cross| < EPSILON * |dir|   <=>   cross² < EPSILON² * |dir|²
     * Both sides of the middle form are non-negative, so squaring keeps the comparison.
     * False for dir = 0 (0 < 0), so a degenerate line contains no point here.
     */
    constexpr bool isOnLine() const noexcept {
        return cross * cross < squaredEpsilon * squaredLength;
    }

    /**
     * @brief Projection between p and p + dir, allowing EPSILON beyond each end.
     *
     * The position along the line is lambda = dot / |dir|² (0 at p, 1 at p + dir). A tolerance of
     * EPSILON in coordinates is EPSILON / |dir| in lambda, so the check is
     *   -EPSILON / |dir| <= lambda <= 1 + EPSILON / |dir|
     * and multiplied by |dir|²:
     *   -EPSILON * |dir| <= dot <= |dir|² + EPSILON * |dir|
     * Written as two overshoots, each scaled by |dir| like dot:
     *   beforeStart = -dot           (> 0 only if the projection lies before p)
     *   pastEnd     = dot - |dir|²   (> 0 only if the projection lies past p + dir)
     * each must be <= EPSILON * |dir|. To avoid the sqrt in |dir|, both sides are squared, which is
     * only valid if both are non-negative (-5 <= 1, but 25 <= 1 is false). So an overshoot <= 0,
     * the common case, passes without comparing; only a positive one is compared squared.
     */
    constexpr bool isBetweenEnds() const noexcept {
        return isNotBeforeStart() and isNotPastEnd();
    }

    /// projection not before p, up to EPSILON: beforeStart = -dot <= EPSILON * |dir| (see isBetweenEnds)
    constexpr bool isNotBeforeStart() const noexcept {
        return isWithinTolerance(-dot);
    }

    /// projection not past p + dir, up to EPSILON: pastEnd = dot - |dir|² <= EPSILON * |dir| (see isBetweenEnds)
    constexpr bool isNotPastEnd() const noexcept {
        return isWithinTolerance(dot - squaredLength);
    }

private:
    /// overshoot <= EPSILON * |dir|, compared squared only when positive (see isBetweenEnds)
    constexpr bool isWithinTolerance(FLOAT_TYPE overshoot) const noexcept {
        return overshoot <= 0 or overshoot * overshoot <= squaredEpsilon * squaredLength;
    }
};
} // namespace __impl

/**
 * @brief Computation of line intersection
 * @param l line object
 * @param r line object
 * @return fishnet::Option<Vec2DReal>: empty optional if lines are parallel, otherwise the intersection of the lines l and r
 * 
 */
constexpr static fishnet::Option<Vec2DReal> inline linearIntersection(ILine auto const& l, ILine auto const & r) noexcept {
    if (l.isParallel(r)) 
        return {};
    auto dThis = Vec2DReal(l.direction());
    const auto & p = l.p();
    const auto & q = l.q();
    const auto & s = r.p();
    const auto & t = r.q();
    using namespace fishnet::math;
    DEFAULT_FLOATING_POINT denominator = (p.x - q.x) * (s.y - t.y) - (p.y - q.y) *(s.x - t.x);
    DEFAULT_FLOATING_POINT lambda = ((p.x - s.x) * (s.y - t.y) - (p.y - s.y) * (s.x - t.x)) /denominator;
    Vec2DReal intersectionOfLines =  p + (dThis * lambda);
    return intersectionOfLines;
}

/**
 * @brief Generic wrapper for intersections between linear features
 * 
 * @param lhs linear geometry
 * @param rhs linear geometry
 * @return fishnet::Option<Vec2DReal>
 */
constexpr static fishnet::Option<Vec2DReal> inline linearIntersection(LinearGeometry auto const& lhs, LinearGeometry auto const& rhs) requires(!ILine<decltype(lhs)> || !ILine<decltype(rhs)>) {
    return linearIntersection(lhs.toLine(),rhs.toLine()).filter([&lhs,&rhs](const auto & intersection){return lhs.contains(intersection) && rhs.contains(intersection);});
}

constexpr static bool inline areParallel(LinearGeometry auto const & lhs, LinearGeometry auto const & rhs) noexcept {
    return lhs.direction().isParallel(rhs.direction());
}

constexpr static bool intersect(LinearGeometry auto const & lhs,LinearGeometry auto const& rhs) noexcept {
    return linearIntersection(lhs,rhs).has_value();
}
}