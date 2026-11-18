#pragma once
#include <fishnet/Vec2D.hpp>
#include <fishnet/IGeometry.hpp>
#include <fishnet/FunctionalConcepts.hpp>
#include "SweepLine.hpp"
#include <fishnet/Segment.hpp>

namespace fishnet::geometry {

/**
 * @brief Compute the point on the segment, closest to the query point:
 * the projection of the point onto the segment's line, clamped to the segment.
 * lambda = dot(point - p, dir) / |dir|² is the position of the projection (0 at p, 1 at q); outside
 * [0,1] the nearer endpoint is the closest point, which is what clamping lambda selects (hot path).
 *
 * @param segment
 * @param point
 * @return
 */
static Vec2DReal closestPointOnSegment(ISegment auto const & segment, IPoint auto const & point) noexcept {
        auto dir = segment.direction();
        auto position = __impl::PointOnLinePosition(segment.p(), dir, point);
        if(position.squaredLength == 0) // 0-length segment
            return segment.p();
        auto lambda = std::clamp(position.dot / position.squaredLength, 0.0, 1.0);
        return segment.p() + dir * lambda;
}


/**
 * @brief Compute the pair of nearest points for two segments
 * 
 * @param lhs 
 * @param rhs 
 * @return closest pair of points (first point from lhs segment, second point from rhs segment)
 */
static std::pair<Vec2DReal,Vec2DReal> closestPoints(ISegment auto const & lhs, ISegment auto const & rhs) noexcept {
    std::array<std::pair<Vec2DReal,Vec2DReal>,4> points {{
        {closestPointOnSegment(lhs,rhs.p()),rhs.p()},
        {closestPointOnSegment(lhs,rhs.q()),rhs.q()},
        {lhs.p(),closestPointOnSegment(rhs,lhs.p())},
        {lhs.q(),closestPointOnSegment(rhs,lhs.q())}
    }};
    return *std::ranges::min_element(points,[](const auto & left, const auto & right){
        const auto & [lFrom,lTo] = left;
        const auto & [rFrom,rTo] = right;
        return lFrom.distance(lTo) < rFrom.distance(rTo);
    });
}

namespace __impl{

class ClosestPointsResult;

/**
 * @brief Helper class for referencing the associated polygon of segment / point during sweep line
 * 
 */
class PolygonReference{
private:
    friend class ClosestPointsResult;
    enum class Type{
        THIS,OTHER,NONE
    }; 
    Type type;

    PolygonReference():type(Type::NONE){}
    PolygonReference(Type type):type(type){}

    constexpr bool inline isNone() const noexcept {
        return this->type==Type::NONE;
    }
public:
    static inline PolygonReference THIS() {
        return PolygonReference(Type::THIS);
    }

    static PolygonReference OTHER() {
        return PolygonReference(Type::OTHER);
    }

    constexpr bool inline operator<(const PolygonReference & other) const noexcept {
        return type < other.type;
    }

    constexpr bool inline operator==(const PolygonReference & other) const noexcept {
        return type == other.type;
    }

    PolygonReference other() const noexcept {
        if(type == Type::THIS)
            return PolygonReference::OTHER();
        else
            return PolygonReference::THIS();
    }

};

/**
 * @brief Input type of the Polygon Segment sweep
 * Stores the segment and holds polygon reference to its associated polygon
 * 
 * @tparam isXOrdered indicates whether the points are ordered by x or y coordinate first
 */
template<bool isXOrdered>
struct PolygonPoint {
    constexpr static LexicographicOrder xOrdering {};
    constexpr static YLexicographicOrder yOrdering{};
    Segment<fishnet::math::DEFAULT_FLOATING_POINT> segment;
    PolygonReference polygonRef;

    PolygonPoint(Segment<fishnet::math::DEFAULT_FLOATING_POINT> && segment, PolygonReference polygonReference)
    :segment(std::move(segment)),polygonRef(polygonReference){}

    constexpr const auto & getPoint() const noexcept {
        return segment.p();
    }

    constexpr const auto & getSegment() const noexcept {
        return segment;
    }

    constexpr bool inline operator<(const PolygonPoint& other ) const noexcept {
        if(polygonRef==other.polygonRef){
            if constexpr(isXOrdered)
                return xOrdering(this->getPoint(),other.getPoint());
            else
                return yOrdering(this->getPoint(),other.getPoint());
        }
        return polygonRef < other.polygonRef;
    }
};

/**
 * @brief Result of Polygon Segment Sweep line
 * Keeps track of the minimum distance and the closest pair of points
 * Additionally tracks whether the other polygon was found by the sweep line and if one polygon was already fully processed.
 * This avoids unnecessary distance computations
 */
class ClosestPointsResult{
private:
    bool foundPointOfOtherPolygon = false;
    PolygonReference polyRef;
public:
    Vec2DReal thisPoint;
    Vec2DReal otherPoint;
    size_t thisCounter = 0;
    size_t otherCounter = 0;
    fishnet::math::DEFAULT_FLOATING_POINT minDistance;

    ClosestPointsResult(const Vec2DReal & thisInitial, const Vec2DReal & otherInitial,size_t thisCounter, size_t otherCounter)
    :thisPoint(thisInitial),otherPoint(otherInitial),thisCounter(thisCounter),otherCounter(otherCounter),minDistance(thisInitial.distance(otherInitial)){}

    bool startSearch(PolygonReference polygonRef){
        if(foundPointOfOtherPolygon){
            return true;
        }
        [[unlikely]] if(polyRef.isNone()){// only the case for the first element
            polyRef = polygonRef; 
        }
        if(polyRef != polygonRef){
            foundPointOfOtherPolygon = true;
        }
        return foundPointOfOtherPolygon;
    }

    /**
     * @brief Updates the amount of points to be left for processing
     * 
     * @tparam O 
     * @param point 
     */
    template<bool O>
    void inline visit(const PolygonPoint<O> & point) noexcept {
        if(point.polygonRef == PolygonReference::THIS())
            thisCounter--;
        else 
            otherCounter--;
    }

    /**
     * @brief Get the current point of other polygon which is part of the closest pair
     * 
     * @tparam O 
     * @param point 
     * @return const Vec2DReal& 
     */
    template<bool O>
    const Vec2DReal & getNeighbour(const PolygonPoint<O> & point) const noexcept {
        if(point.polygonRef == PolygonReference::THIS()){
            return otherPoint;
        }
        return thisPoint;
    }
};

template<bool isXOrdered>
using PolygonPointSweepLine = SweepLine<PolygonPoint<isXOrdered>,ClosestPointsResult,std::less<PolygonPoint<isXOrdered>>,false,std::less<fishnet::math::DEFAULT_NUMERIC>>;

template<bool isXOrdered>
struct PolygonSegmentSweepEvent : public PolygonPointSweepLine<isXOrdered>::InsertEvent {

    PolygonSegmentSweepEvent(const PolygonPoint<isXOrdered> & point):PolygonPointSweepLine<isXOrdered>::InsertEvent(point){}

    virtual fishnet::math::DEFAULT_NUMERIC eventPoint() const noexcept {
        if constexpr(isXOrdered)
            return this->obj->getPoint().x;
        else 
            return this->obj->getPoint().y;
    }

    /**
     * @brief Indicates whether the sweep line can be stop.
     * This is the case when one polygon was fully processed and the current segment is out of range of the closest possible neighbour
     * @param sweepLine 
     * @param status 
     * @return true 
     * @return false 
     */
    bool inline stop(PolygonPointSweepLine<isXOrdered> & sweepLine, ClosestPointsResult & status) const {
        if constexpr(isXOrdered)
            return (status.thisCounter==0 || status.otherCounter==0) && this->obj->segment.leftEndpoint().x - status.minDistance > status.getNeighbour(*this->obj).x;
        else 
            return (status.thisCounter==0 || status.otherCounter==0) && this->obj->segment.lowerEndpoint().x - status.minDistance > status.getNeighbour(*this->obj).x;
    }

    /**
     * @brief Query for finding segments in the buffer of the sweep line (currentSegment.left.x-distance,current.x]
     * x is swapped for y when sweep line is not ordered by x
     * @param status 
     * @return PolygonPoint<isXOrdered>
     */
    PolygonPoint<isXOrdered> lowerBoundQuery(ClosestPointsResult & status) const noexcept {
        if constexpr(isXOrdered)
            return PolygonPoint<isXOrdered>(Segment<fishnet::math::DEFAULT_FLOATING_POINT>(this->obj->segment.leftEndpoint() - Vec2DReal(status.minDistance,0),this->obj->getPoint()),this->obj->polygonRef.other());
        else 
            return PolygonPoint<isXOrdered>(Segment<fishnet::math::DEFAULT_FLOATING_POINT>(this->obj->segment.lowerEndpoint() - Vec2DReal(0,status.minDistance),this->obj->getPoint()),this->obj->polygonRef.other());
    }

    /**
     * @brief Query for finding segments in the buffer of the sweep line [current.x,currentSegment.right.x+distance)
     * x is swapped for y when sweep line is not ordered by x
     * @param status 
     * @return PolygonPoint<isXOrdered> 
     */
    PolygonPoint<isXOrdered> upperBoundQuery(ClosestPointsResult & status) const noexcept {
        if constexpr(isXOrdered)
            return PolygonPoint<isXOrdered>({this->obj->segment.rightEndpoint() + Vec2DReal(status.minDistance,0),this->obj->getPoint()},this->obj->polygonRef.other());
        else 
            return PolygonPoint<isXOrdered>({this->obj->segment.upperEndpoint() + Vec2DReal(0,status.minDistance),this->obj->getPoint()},this->obj->polygonRef.other());
    }

    virtual void process(PolygonPointSweepLine<isXOrdered> & sweepLine, ClosestPointsResult & status) const {
        const auto & polygonPoint = *this->obj;
        const auto & point = polygonPoint.getPoint();
        const auto & segment = polygonPoint.getSegment();
        sweepLine.addSLS(this->obj);
        status.visit(polygonPoint);
        if(stop(sweepLine,status)){
            /*Clear Event Queue since no closer distances can be found*/
            while(sweepLine.getEventQueue().size() > 1)
                sweepLine.getEventQueue().pop();
            return;
        }
        if(not status.startSearch(polygonPoint.polygonRef))
            return;
        auto leftQuery  = lowerBoundQuery(status);
        auto rightQuery = upperBoundQuery(status);
        auto lowerBound = sweepLine.lowerBound(leftQuery);
        auto upperBound = sweepLine.upperBound(rightQuery);
        auto & sweepLineStatus = sweepLine.getSLS();
        for(auto it= lowerBound; it != upperBound;){
            const auto & currentSegment = (*it)->getSegment();
            /*Remove elements out of range from the sweep line*/
            if constexpr(isXOrdered){
                if(currentSegment.rightEndpoint().x + status.minDistance < point.x){
                    it = sweepLineStatus.erase(it);
                    continue;
                }
            }else {
                if(currentSegment.upperEndpoint().y + status.minDistance < point.y){
                    it = sweepLineStatus.erase(it);
                    continue;
                }
            }
            auto [l,r] = closestPoints(segment,currentSegment);
            auto distance = l.distance(r);
            if(distance < status.minDistance){
                status.minDistance = distance;
                /*Ensure the points in status are set for the correct polygon*/
                if(polygonPoint.polygonRef == PolygonReference::THIS()){
                    status.thisPoint = l;
                    status.otherPoint = r;
                } else {
                    status.thisPoint = r;
                    status.otherPoint = l;
                }
            }
            ++it;
        }
    }
};

/**
 * @brief Sweep line approach for computing the closest pair of points for two segment ranges
 * 
 * @tparam xSweep true -> sweep left to right, false -> sweep bottom to top
 * @param lhs segment range of a shape
 * @param rhs segment range of another shape
 * @return std::pair<Vec2DReal,Vec2DReal> closest pair of points
 */
template<bool xSweep>
static std::pair<Vec2DReal,Vec2DReal> closestPointsSweep(SegmentRange auto && lhs, SegmentRange auto && rhs) noexcept {
    auto lInit = Vec2DReal((*std::ranges::begin(lhs)).p());
    auto rInit = Vec2DReal((*std::ranges::begin(rhs)).p());
    PolygonPointSweepLine<xSweep> sweepLine;
    /*Initialize status with two "random" points of the segment ranges*/
    ClosestPointsResult status {lInit,rInit,fishnet::util::size(lhs),fishnet::util::size(rhs)};
    std::vector<PolygonPoint<xSweep>> segments;
    std::ranges::for_each(lhs,[&segments,&status](const auto & segmentLeft){
        if constexpr(xSweep){
            segments.push_back(PolygonPoint<xSweep>(Segment<fishnet::math::DEFAULT_FLOATING_POINT>(Vec2DReal(segmentLeft.leftEndpoint()),Vec2DReal(segmentLeft.rightEndpoint())),PolygonReference::THIS()));
            segments.push_back(PolygonPoint<xSweep>(Segment<fishnet::math::DEFAULT_FLOATING_POINT>(Vec2DReal(segmentLeft.rightEndpoint()),Vec2DReal(segmentLeft.leftEndpoint())),PolygonReference::THIS()));
        }else {
            segments.push_back(PolygonPoint<xSweep>(Segment<fishnet::math::DEFAULT_FLOATING_POINT>(Vec2DReal(segmentLeft.lowerEndpoint()),Vec2DReal(segmentLeft.upperEndpoint())),PolygonReference::THIS()));
            segments.push_back(PolygonPoint<xSweep>(Segment<fishnet::math::DEFAULT_FLOATING_POINT>(Vec2DReal(segmentLeft.upperEndpoint()),Vec2DReal(segmentLeft.lowerEndpoint())),PolygonReference::THIS()));
        }
        status.thisCounter+=2; // keep track of points to be processed -> allows early return
    });
    std::ranges::for_each(rhs,[&segments,&status](const auto & segmentRight){
        if constexpr(xSweep){
            segments.push_back(PolygonPoint<xSweep>(Segment<fishnet::math::DEFAULT_FLOATING_POINT>(Vec2DReal(segmentRight.leftEndpoint()),Vec2DReal(segmentRight.rightEndpoint())),PolygonReference::OTHER()));
            segments.push_back(PolygonPoint<xSweep>(Segment<fishnet::math::DEFAULT_FLOATING_POINT>(Vec2DReal(segmentRight.rightEndpoint()),Vec2DReal(segmentRight.leftEndpoint())),PolygonReference::OTHER()));
        }else {
            segments.push_back(PolygonPoint<xSweep>(Segment<fishnet::math::DEFAULT_FLOATING_POINT>(Vec2DReal(segmentRight.lowerEndpoint()),Vec2DReal(segmentRight.upperEndpoint())),PolygonReference::OTHER()));
            segments.push_back(PolygonPoint<xSweep>(Segment<fishnet::math::DEFAULT_FLOATING_POINT>(Vec2DReal(segmentRight.upperEndpoint()),Vec2DReal(segmentRight.lowerEndpoint())),PolygonReference::OTHER()));
        }
        status.otherCounter+=2;
    });
    std::ranges::for_each(segments,[&sweepLine](const auto & segment){
        sweepLine.addEvent(std::make_unique<PolygonSegmentSweepEvent<xSweep>>(segment));
    });
    auto result = sweepLine.sweep(status);
    return {result.thisPoint,result.otherPoint};
}


static std::pair<Vec2DReal,Vec2DReal> closestPointsSweep(SegmentRange auto && lhs, SegmentRange auto && rhs) noexcept {
    auto lInit = Vec2DReal((*std::ranges::begin(lhs)).p());
    auto rInit = Vec2DReal((*std::ranges::begin(rhs)).p());
    auto dirVector = rInit - lInit;
    if(fabs(dirVector.x) > fabs(dirVector.y)){
        return closestPointsSweep<true>(lhs,rhs);
    }
    return closestPointsSweep<false>(lhs,rhs);
}


/**
 * @brief Compute the pair of closest points of two segment ranges belong to different shapes
 * 
 * @param lhs ring object
 * @param rhs ring object
 * @return std::pair<Vec2DReal,Vec2DReal> 
 */
static std::pair<Vec2DReal,Vec2DReal> closestPointsBruteForce(SegmentRange auto && lhs,SegmentRange auto  && rhs) noexcept {
    fishnet::math::DEFAULT_FLOATING_POINT minDistance = std::numeric_limits<fishnet::math::DEFAULT_FLOATING_POINT>::max();
    Vec2DReal bestLeft;
    Vec2DReal bestRight;
    auto leftPoints = lhs | std::views::transform([](const auto & s){return s.p();});
    auto rightPoints = rhs | std::views::transform([](const auto & s){return s.p();});
    for(const auto & s : lhs){
        for(const auto & p : rightPoints){
            auto pointOnSegment = closestPointOnSegment(s,p);
            if(pointOnSegment.distance(p) < minDistance){
                bestLeft = pointOnSegment;
                bestRight = p;
                minDistance = pointOnSegment.distance(p);
            }
        }
    }
    for(const auto & s : rhs){
        for(const auto & p: leftPoints){
            auto pointOnSegment = closestPointOnSegment(s,p);
            if(pointOnSegment.distance(p) < minDistance){
                bestLeft = p;
                bestRight = pointOnSegment;
                minDistance = pointOnSegment.distance(p);
            }
        }
    }
    return std::make_pair(bestLeft,bestRight);
}

/**
 * @brief Default distance function, which uses the distance between the vectors 
 * The result is in the same units, as the vectors
 */
struct DefaultDistanceFunction{
    template<typename T,typename U>
    static inline fishnet::math::DEFAULT_FLOATING_POINT operator()(const Vec2D<T> & lhs, const Vec2D<U> & rhs) noexcept {
        return lhs.distance(rhs);
    }
};

/**
 * @brief Generic SegmentRange overload to compute the closest pair of points of two shapes
 * This function uses the solely the range of segments of the shape for the closest points, assuming that the shapes induced by the segments do not contain each other.
 * @param lhs ring
 * @param rhs other ring
 * @return std::pair<Vec2DReal,Vec2DReal> closest pair
 */
static std::pair<Vec2DReal,Vec2DReal> closestPoints( SegmentRange auto && lhs,  SegmentRange auto && rhs) noexcept {
    constexpr static size_t SWEEP_LINE_THRESHOLD = 10000;
    if(fishnet::util::size(lhs) * fishnet::util::size(rhs) > SWEEP_LINE_THRESHOLD){
        return __impl::closestPointsSweep(lhs,rhs);
    }
    return __impl::closestPointsBruteForce(lhs,rhs);
}
}

/**
 * @brief Ring overload to compute the closest pair of points
 * This function uses the segment on the boundary of the shape to compute the closest pair of points, assuming that the rings do not contain each other.
 * @param lhs ring
 * @param rhs other ring
 * @return std::pair<Vec2DReal,Vec2DReal> closest pair
 */
static std::pair<Vec2DReal,Vec2DReal> closestPoints(const IRing auto & lhs, const IRing auto & rhs) noexcept {
    return __impl::closestPoints(lhs.getSegments(),rhs.getSegments());
}

/**
 * @brief Overload for polygons to compute the closest pair of points
 * 
 * @param lhs polygon
 * @param rhs other polygon
 * @return std::pair<Vec2DReal,Vec2DReal> closest pair
 */
template <Shape S, Shape T, typename = std::enable_if_t<IPolygon<S> || IPolygon<T>>>
static std::pair<Vec2DReal,Vec2DReal> closestPoints(const S  & lhs, const T   & rhs) noexcept {
    return closestPoints(lhs.getBoundary(),rhs.getBoundary());
}

/**
 * @brief Overload for multiPolygons to compute the closest pair of points
 * 
 * @param lhs multi-polygon
 * @param rhs other multi-polygon
 * @return std::pair<Vec2DReal,Vec2DReal> closest pair
 */
static std::pair<Vec2DReal,Vec2DReal> closestPoints(const IMultiPolygon auto & lhs, const IMultiPolygon auto & rhs) noexcept {
    return __impl::closestPoints(
        lhs.getPolygons()|std::views::transform([](const auto & polygon){return polygon.getBoundary().getSegments();}) | std::views::join,
        rhs.getPolygons()|std::views::transform([](const auto & polygon){return polygon.getBoundary().getSegments();}) | std::views::join
    );
}


/**
 * @brief Compute the distance between two shapes, with custom distance function
 * 
 * @param lhs 
 * @param rhs 
 * @param distanceFunction custom distance function. Allows computation in other units (e.g. meters instead of angular units)
 * @return fishnet::math::DEFAULT_FLOATING_POINT 
 */
static fishnet::math::DEFAULT_FLOATING_POINT shapeDistance(Shape auto const & lhs, Shape auto const & rhs,util::BiFunction<Vec2DReal,Vec2DReal,fishnet::math::DEFAULT_FLOATING_POINT> auto const & distanceFunction) noexcept {
    const auto & [l,r] = closestPoints(lhs,rhs);
    return distanceFunction(l,r);
}

/**
 * @brief Default shapeDistance overload
 * 
 * @param lhs 
 * @param rhs 
 * @return fishnet::math::DEFAULT_FLOATING_POINT 
 */
static fishnet::math::DEFAULT_FLOATING_POINT shapeDistance(Shape auto const & lhs, Shape auto const & rhs) noexcept {
    return shapeDistance(lhs,rhs,__impl::DefaultDistanceFunction());
}
}