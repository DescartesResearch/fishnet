#include <gtest/gtest.h>
#include <fishnet/TestUtil.hpp>
#include <fishnet/OGRPolygonAdapter.hpp>
#include <fishnet/Polygon.hpp>
#include <fishnet/SimplePolygon.hpp>
#include <fishnet/Ring.hpp>
#include <fishnet/Line.hpp>

using namespace fishnet::geometry;
using namespace fishnet::test;

/**
 * The OGRPolygonAdapter is expected to be substitutable for fishnet::geometry::Polygon, so most
 * assertions below are cross checked against the equivalent Polygon<double>.
 */
class OGRPolygonAdapterTest : public ::testing::Test {
protected:
    // 10x10 square with a 2x2 hole in its middle
    static Ring<double> outerRing() {
        return Ring<double>(std::vector<Vec2DReal>{{0,0},{0,10},{10,10},{10,0}});
    }
    static Ring<double> holeRing() {
        return Ring<double>(std::vector<Vec2DReal>{{4,4},{4,6},{6,6},{6,4}});
    }
    static Polygon<double> referenceWithHole() {
        return Polygon<double>(outerRing(), std::vector<Ring<double>>{holeRing()});
    }
    static Polygon<double> referenceSimple() {
        return Polygon<double>(outerRing());
    }

    void SetUp() override {
        withHole = std::make_unique<OGRPolygonAdapter>(referenceWithHole());
        simple = std::make_unique<OGRPolygonAdapter>(referenceSimple());
    }

    std::unique_ptr<OGRPolygonAdapter> withHole;
    std::unique_ptr<OGRPolygonAdapter> simple;
};

// ============================================================================
// Construction
// ============================================================================

TEST_F(OGRPolygonAdapterTest, initFromPolygon) {
    OGRPolygonAdapter adapted(referenceWithHole());
    EXPECT_DOUBLE_EQ(adapted.area(), referenceWithHole().area());
    EXPECT_SIZE(adapted.getHoles(), 1);
}

TEST_F(OGRPolygonAdapterTest, initFromRingProducesSimplePolygon) {
    OGRPolygonAdapter adapted{Polygon<double>(outerRing())};
    EXPECT_TRUE(adapted.isSimple());
    EXPECT_EMPTY(adapted.getHoles());
    EXPECT_DOUBLE_EQ(adapted.area(), 100.0);
}

TEST_F(OGRPolygonAdapterTest, initFromSimplePolygon) {
    SimplePolygon<double> simplePolygon(outerRing());
    OGRPolygonAdapter adapted(simplePolygon);
    EXPECT_TRUE(adapted.isSimple());
    EXPECT_DOUBLE_EQ(adapted.area(), 100.0);
}

TEST_F(OGRPolygonAdapterTest, initFromBoundaryAndHoles) {
    OGRPolygonAdapter adapted{Polygon<double>(outerRing(), std::vector<Ring<double>>{holeRing()})};
    EXPECT_FALSE(adapted.isSimple());
    EXPECT_SIZE(adapted.getHoles(), 1);
    EXPECT_DOUBLE_EQ(adapted.area(), 96.0);
}

TEST_F(OGRPolygonAdapterTest, initFromOGRPtr) {
    OGRLinearRing ring;
    ring.addPoint(0,0);
    ring.addPoint(0,10);
    ring.addPoint(10,10);
    ring.addPoint(10,0);
    ring.closeRings();
    auto * ogrPolygon = static_cast<OGRPolygon*>(OGRGeometryFactory::createGeometry(wkbPolygon));
    ogrPolygon->addRing(&ring);
    auto ptr = OGRUniquePtr<OGRPolygon>(ogrPolygon);
    OGRPolygonAdapter adapted(std::move(ptr));
    EXPECT_DOUBLE_EQ(adapted.area(), 100.0);
    EXPECT_TRUE(adapted.isSimple());
}

TEST_F(OGRPolygonAdapterTest, copyAndMove) {
    OGRPolygonAdapter copy(*withHole);
    EXPECT_EQ(copy, *withHole);
    OGRPolygonAdapter moved(std::move(copy));
    EXPECT_EQ(moved, *withHole);
}

// ============================================================================
// Accessors
// ============================================================================

TEST_F(OGRPolygonAdapterTest, getBoundary) {
    EXPECT_EQ(withHole->getBoundary(), OGRRingAdapter(outerRing()));
    // exercises the rvalue overload of getPoints(): getBoundary() hands back a temporary here
    EXPECT_SORTED_RANGE_EQ(withHole->getBoundary().getPoints(), outerRing().getPoints());
}

TEST_F(OGRPolygonAdapterTest, getHoles) {
    EXPECT_SIZE(withHole->getHoles(), 1);
    EXPECT_EQ(withHole->getHoles().front(), OGRRingAdapter(holeRing()));
    EXPECT_EMPTY(simple->getHoles());
}

TEST_F(OGRPolygonAdapterTest, getHolesWithMultipleHoles) {
    Ring<double> secondHole(std::vector<Vec2DReal>{{1,1},{1,2},{2,2},{2,1}});
    OGRPolygonAdapter adapted{Polygon<double>(outerRing(), std::vector<Ring<double>>{holeRing(), secondHole})};
    EXPECT_SIZE(adapted.getHoles(), 2);
    EXPECT_DOUBLE_EQ(adapted.area(), 100.0 - 4.0 - 1.0);
}

TEST_F(OGRPolygonAdapterTest, isSimple) {
    EXPECT_TRUE(simple->isSimple());
    EXPECT_FALSE(withHole->isSimple());
}

// ============================================================================
// Geometry properties
// ============================================================================

TEST_F(OGRPolygonAdapterTest, area) {
    EXPECT_DOUBLE_EQ(simple->area(), 100.0);
    EXPECT_DOUBLE_EQ(withHole->area(), 96.0);  // hole is subtracted
}

TEST_F(OGRPolygonAdapterTest, areaMatchesPolygon) {
    EXPECT_DOUBLE_EQ(simple->area(), referenceSimple().area());
    EXPECT_DOUBLE_EQ(withHole->area(), referenceWithHole().area());
}

TEST_F(OGRPolygonAdapterTest, centroidMatchesPolygon) {
    EXPECT_EQ(simple->centroid(), referenceSimple().centroid());
    EXPECT_EQ(withHole->centroid(), referenceWithHole().centroid());
}

TEST_F(OGRPolygonAdapterTest, centroidOfSymmetricPolygon) {
    EXPECT_EQ(simple->centroid(), Vec2DReal(5,5));
    // a centred hole does not move the centroid
    EXPECT_EQ(withHole->centroid(), Vec2DReal(5,5));
}

TEST_F(OGRPolygonAdapterTest, centroidOfSliver) {
    // building footprint in WGS84 with an area of ~1e-13, below epsilon once multiplied with the centroid
    OGRPolygonAdapter sliver {Polygon<double>(Ring<double>(std::vector<Vec2DReal>{{32.85259276854312, -3.9553591931308945},{32.8525946353636, -3.95535907994086},{32.85259464216783, -3.9553591931308945}}))};
    auto centroid = sliver.centroid();
    EXPECT_NEAR(centroid.x, 32.852594, 1e-6);
    EXPECT_NEAR(centroid.y, -3.955359, 1e-6);
}

TEST_F(OGRPolygonAdapterTest, aaBB) {
    auto bbox = withHole->aaBB();
    EXPECT_DOUBLE_EQ(bbox.left(), 0.0);
    EXPECT_DOUBLE_EQ(bbox.right(), 10.0);
    EXPECT_DOUBLE_EQ(bbox.top(), 10.0);
    EXPECT_DOUBLE_EQ(bbox.bottom(), 0.0);
}

// ============================================================================
// Point containment (hole aware)
// ============================================================================

TEST_F(OGRPolygonAdapterTest, isInside) {
    EXPECT_TRUE(withHole->isInside(Vec2DReal(1,1)));
    EXPECT_FALSE(withHole->isInside(Vec2DReal(5,5)));    // in the hole
    EXPECT_FALSE(withHole->isInside(Vec2DReal(0,0)));    // on the boundary
    EXPECT_FALSE(withHole->isInside(Vec2DReal(-1,-1)));  // outside
}

TEST_F(OGRPolygonAdapterTest, isOnBoundary) {
    EXPECT_TRUE(withHole->isOnBoundary(Vec2DReal(0,0)));   // vertex of the exterior ring
    EXPECT_TRUE(withHole->isOnBoundary(Vec2DReal(5,0)));   // edge of the exterior ring
    EXPECT_TRUE(withHole->isOnBoundary(Vec2DReal(4,4)));   // vertex of the hole
    EXPECT_TRUE(withHole->isOnBoundary(Vec2DReal(5,4)));   // edge of the hole
    EXPECT_FALSE(withHole->isOnBoundary(Vec2DReal(1,1)));  // interior
    EXPECT_FALSE(withHole->isOnBoundary(Vec2DReal(5,5)));  // inside the hole
}

TEST_F(OGRPolygonAdapterTest, isOutside) {
    EXPECT_TRUE(withHole->isOutside(Vec2DReal(-1,-1)));
    EXPECT_TRUE(withHole->isOutside(Vec2DReal(5,5)));      // the hole is not part of the polygon
    EXPECT_FALSE(withHole->isOutside(Vec2DReal(1,1)));
    EXPECT_FALSE(withHole->isOutside(Vec2DReal(0,0)));
}

TEST_F(OGRPolygonAdapterTest, pointLocationIsExhaustive) {
    std::vector<Vec2DReal> probes {
        {1,1},{5,5},{0,0},{5,0},{4,4},{-1,-1},{9.9,9.9},{5,4}
    };
    for(const auto & probe : probes){
        int locations = int(withHole->isInside(probe)) + int(withHole->isOnBoundary(probe)) + int(withHole->isOutside(probe));
        EXPECT_EQ(locations, 1) << "Point " << probe.toString() << " must have exactly one location";
    }
}

TEST_F(OGRPolygonAdapterTest, containsPoint) {
    EXPECT_TRUE(withHole->contains(Vec2DReal(1,1)));    // interior
    EXPECT_TRUE(withHole->contains(Vec2DReal(0,0)));    // boundary counts
    EXPECT_TRUE(withHole->contains(Vec2DReal(4,4)));    // hole boundary counts
    EXPECT_FALSE(withHole->contains(Vec2DReal(5,5)));   // inside the hole
    EXPECT_FALSE(withHole->contains(Vec2DReal(-1,-1))); // outside
}

TEST_F(OGRPolygonAdapterTest, containsInHolePoint) {
    EXPECT_TRUE(withHole->containsInHole(Vec2DReal(5,5)));
    EXPECT_FALSE(withHole->containsInHole(Vec2DReal(1,1)));
    EXPECT_FALSE(simple->containsInHole(Vec2DReal(5,5)));
}

TEST_F(OGRPolygonAdapterTest, containsInHolePolygon) {
    Polygon<double> inHole(Ring<double>(std::vector<Vec2DReal>{{4.5,4.5},{4.5,5.5},{5.5,5.5},{5.5,4.5}}));
    EXPECT_TRUE(withHole->containsInHole(inHole));
    EXPECT_FALSE(simple->containsInHole(inHole));

    Polygon<double> outsideHole(Ring<double>(std::vector<Vec2DReal>{{1,1},{1,2},{2,2},{2,1}}));
    EXPECT_FALSE(withHole->containsInHole(outsideHole));
}

// ============================================================================
// Segment / ring / polygon containment
// ============================================================================

TEST_F(OGRPolygonAdapterTest, containsSegment) {
    EXPECT_TRUE(withHole->contains(Segment(Vec2DReal(1,1), Vec2DReal(3,3))));       // fully inside
    EXPECT_TRUE(withHole->contains(Segment(Vec2DReal(0,0), Vec2DReal(0,10))));      // along the boundary
    EXPECT_FALSE(withHole->contains(Segment(Vec2DReal(1,5), Vec2DReal(9,5))));      // passes through the hole
    EXPECT_FALSE(withHole->contains(Segment(Vec2DReal(-1,1), Vec2DReal(1,1))));     // partially outside
    EXPECT_FALSE(withHole->contains(Segment(Vec2DReal(20,20), Vec2DReal(30,30))));  // fully outside
    EXPECT_TRUE(simple->contains(Segment(Vec2DReal(1,5), Vec2DReal(9,5))));         // no hole in the way
}

TEST_F(OGRPolygonAdapterTest, containsRing) {
    Ring<double> insideRing(std::vector<Vec2DReal>{{1,1},{1,2},{2,2},{2,1}});
    EXPECT_TRUE(withHole->contains(insideRing));

    Ring<double> aroundHole(std::vector<Vec2DReal>{{3,3},{3,7},{7,7},{7,3}});
    EXPECT_FALSE(withHole->contains(aroundHole)); // would swallow the hole
    EXPECT_TRUE(simple->contains(aroundHole));

    Ring<double> outsideRing(std::vector<Vec2DReal>{{20,20},{20,21},{21,21},{21,20}});
    EXPECT_FALSE(withHole->contains(outsideRing));
}

TEST_F(OGRPolygonAdapterTest, containsPolygon) {
    Polygon<double> inside(Ring<double>(std::vector<Vec2DReal>{{1,1},{1,2},{2,2},{2,1}}));
    EXPECT_TRUE(withHole->contains(inside));
    EXPECT_EQ(withHole->contains(inside), referenceWithHole().contains(inside));

    Polygon<double> inHole(Ring<double>(std::vector<Vec2DReal>{{4.5,4.5},{4.5,5.5},{5.5,5.5},{5.5,4.5}}));
    EXPECT_FALSE(withHole->contains(inHole));
    EXPECT_EQ(withHole->contains(inHole), referenceWithHole().contains(inHole));

    Polygon<double> outside(Ring<double>(std::vector<Vec2DReal>{{20,20},{20,21},{21,21},{21,20}}));
    EXPECT_FALSE(withHole->contains(outside));

    // a polygon contains itself
    EXPECT_TRUE(withHole->contains(*withHole));
    EXPECT_TRUE(withHole->contains(referenceWithHole()));
}

TEST_F(OGRPolygonAdapterTest, containsPolygonWithSameHole) {
    // the hole of the other polygon does not have to be filled by this one
    EXPECT_TRUE(simple->contains(referenceWithHole()));
}

// ============================================================================
// Linear geometry intersection
// ============================================================================

TEST_F(OGRPolygonAdapterTest, intersects) {
    // a line through the polygon crosses its boundary
    EXPECT_TRUE(withHole->intersects(Line<double>::horizontalLine(1)));
    // ... and a line through the hole crosses the hole's boundary too
    EXPECT_TRUE(withHole->intersects(Line<double>::horizontalLine(5)));
    // a segment strictly inside crosses nothing
    EXPECT_FALSE(withHole->intersects(Segment(Vec2DReal(1,1), Vec2DReal(2,2))));
    // a segment far away crosses nothing
    EXPECT_FALSE(withHole->intersects(Segment(Vec2DReal(20,20), Vec2DReal(30,30))));
    // a segment poking into the hole crosses the hole's boundary
    EXPECT_TRUE(withHole->intersects(Segment(Vec2DReal(1,5), Vec2DReal(5,5))));
}

TEST_F(OGRPolygonAdapterTest, intersectsMatchesPolygon) {
    auto reference = referenceWithHole();
    EXPECT_EQ(withHole->intersects(Line<double>::horizontalLine(1)), reference.intersects(Line<double>::horizontalLine(1)));
    EXPECT_EQ(withHole->intersects(Line<double>::horizontalLine(5)), reference.intersects(Line<double>::horizontalLine(5)));
    EXPECT_EQ(withHole->intersects(Segment(Vec2DReal(1,1), Vec2DReal(2,2))), reference.intersects(Segment(Vec2DReal(1,1), Vec2DReal(2,2))));
    EXPECT_EQ(withHole->intersects(Segment(Vec2DReal(1,5), Vec2DReal(5,5))), reference.intersects(Segment(Vec2DReal(1,5), Vec2DReal(5,5))));
}

TEST_F(OGRPolygonAdapterTest, intersections) {
    // a horizontal line at y=5 meets the exterior ring twice and the hole twice
    auto result = withHole->intersections(Line<double>::horizontalLine(5));
    EXPECT_SIZE(result, 4);
    EXPECT_CONTAINS(result, Vec2DReal(0,5));
    EXPECT_CONTAINS(result, Vec2DReal(10,5));
    EXPECT_CONTAINS(result, Vec2DReal(4,5));
    EXPECT_CONTAINS(result, Vec2DReal(6,5));

    // the simple polygon only has the two boundary intersections
    EXPECT_SIZE(simple->intersections(Line<double>::horizontalLine(5)), 2);

    EXPECT_EMPTY(withHole->intersections(Segment(Vec2DReal(20,20), Vec2DReal(30,30))));
}

// ============================================================================
// Polygon spatial relations
// ============================================================================

TEST_F(OGRPolygonAdapterTest, crosses) {
    Polygon<double> overlapping(Ring<double>(std::vector<Vec2DReal>{{5,-5},{5,5},{15,5},{15,-5}}));
    EXPECT_TRUE(simple->crosses(overlapping));
    EXPECT_EQ(simple->crosses(overlapping), referenceSimple().crosses(overlapping));

    Polygon<double> disjoint(Ring<double>(std::vector<Vec2DReal>{{20,20},{20,21},{21,21},{21,20}}));
    EXPECT_FALSE(simple->crosses(disjoint));
    EXPECT_EQ(simple->crosses(disjoint), referenceSimple().crosses(disjoint));

    // a polygon does not cross itself
    EXPECT_FALSE(simple->crosses(*simple));

    // a polygon inside the hole does not cross
    Polygon<double> inHole(Ring<double>(std::vector<Vec2DReal>{{4.5,4.5},{4.5,5.5},{5.5,5.5},{5.5,4.5}}));
    EXPECT_FALSE(withHole->crosses(inHole));
    EXPECT_EQ(withHole->crosses(inHole), referenceWithHole().crosses(inHole));
}

TEST_F(OGRPolygonAdapterTest, touches) {
    Polygon<double> neighbour(Ring<double>(std::vector<Vec2DReal>{{10,0},{10,10},{20,10},{20,0}}));
    EXPECT_TRUE(simple->touches(neighbour));
    EXPECT_EQ(simple->touches(neighbour), referenceSimple().touches(neighbour));

    Polygon<double> disjoint(Ring<double>(std::vector<Vec2DReal>{{20,20},{20,21},{21,21},{21,20}}));
    EXPECT_FALSE(simple->touches(disjoint));
    EXPECT_EQ(simple->touches(disjoint), referenceSimple().touches(disjoint));

    EXPECT_FALSE(simple->touches(*simple));
}

TEST_F(OGRPolygonAdapterTest, distance) {
    // a contained polygon has no gap to its container
    Polygon<double> inside(Ring<double>(std::vector<Vec2DReal>{{1,1},{1,2},{2,2},{2,1}}));
    EXPECT_DOUBLE_EQ(simple->distance(inside), 0.0); // contained, hence no gap
    EXPECT_DOUBLE_EQ(simple->distance(inside), referenceSimple().distance(inside));

    // touching polygons report 0
    Polygon<double> neighbour(Ring<double>(std::vector<Vec2DReal>{{10,0},{10,10},{20,10},{20,0}}));
    EXPECT_DOUBLE_EQ(simple->distance(neighbour), 0.0);
    EXPECT_DOUBLE_EQ(simple->distance(neighbour), referenceSimple().distance(neighbour));

    // disjoint polygons report their gap
    Polygon<double> away(Ring<double>(std::vector<Vec2DReal>{{12,0},{12,10},{20,10},{20,0}}));
    EXPECT_DOUBLE_EQ(simple->distance(away), 2.0);
    EXPECT_DOUBLE_EQ(simple->distance(away), referenceSimple().distance(away));
}

TEST_F(OGRPolygonAdapterTest, distanceToPolygonInHole) {
    // a polygon sitting in the hole is measured against the hole's boundary
    Polygon<double> inHole(Ring<double>(std::vector<Vec2DReal>{{4.5,4.5},{4.5,5.5},{5.5,5.5},{5.5,4.5}}));
    EXPECT_DOUBLE_EQ(withHole->distance(inHole), referenceWithHole().distance(inHole));
    EXPECT_DOUBLE_EQ(withHole->distance(inHole), 0.5);
}

// Cross-checks the OGRPolygonAdapter-to-OGRPolygonAdapter fast path (native GEOS Distance())
// against the generic IPolygon template path, which the tests above never exercise since they
// always pass a plain Polygon<double> as the argument.
TEST_F(OGRPolygonAdapterTest, distanceAgreesBetweenNativeAndGenericPath) {
    auto check = [](const OGRPolygonAdapter & lhs, const Polygon<double> & rhsNative) {
        OGRPolygonAdapter rhsAdapter(rhsNative);
        EXPECT_DOUBLE_EQ(lhs.distance(rhsAdapter), lhs.distance(rhsNative))
            << "lhs=" << lhs.toString() << " rhs=" << rhsAdapter.toString();
    };

    Polygon<double> inside(Ring<double>(std::vector<Vec2DReal>{{1,1},{1,2},{2,2},{2,1}}));
    check(*simple, inside);

    Polygon<double> neighbour(Ring<double>(std::vector<Vec2DReal>{{10,0},{10,10},{20,10},{20,0}}));
    check(*simple, neighbour);

    Polygon<double> away(Ring<double>(std::vector<Vec2DReal>{{12,0},{12,10},{20,10},{20,0}}));
    check(*simple, away);

    Polygon<double> inHole(Ring<double>(std::vector<Vec2DReal>{{4.5,4.5},{4.5,5.5},{5.5,5.5},{5.5,4.5}}));
    check(*withHole, inHole);

    // touches the hole's rim exactly, rather than sitting strictly inside it
    Polygon<double> touchingHoleRim(Ring<double>(std::vector<Vec2DReal>{{4,4},{4,5},{5,5},{5,4}}));
    check(*withHole, touchingHoleRim);

    // straddles the hole's rim: partially inside the hole, partially in the solid area
    Polygon<double> straddlingHoleRim(Ring<double>(std::vector<Vec2DReal>{{3,4.5},{3,5.5},{5,5.5},{5,4.5}}));
    check(*withHole, straddlingHoleRim);

    // crosses the outer boundary: partially inside, partially outside the whole polygon
    Polygon<double> crossingOuter(Ring<double>(std::vector<Vec2DReal>{{-1,4},{-1,6},{1,6},{1,4}}));
    check(*simple, crossingOuter);
    check(*withHole, crossingOuter);

    // identical polygon
    check(*withHole, referenceWithHole());

    // two disjoint holes: nearest approach is through the closer one
    Polygon<double> outerBig(Ring<double>(std::vector<Vec2DReal>{{0,0},{0,20},{20,20},{20,0}}));
    Ring<double> holeA(std::vector<Vec2DReal>{{2,2},{2,4},{4,4},{4,2}});
    Ring<double> holeB(std::vector<Vec2DReal>{{10,10},{10,12},{12,12},{12,10}});
    OGRPolygonAdapter twoHoles{Polygon<double>(outerBig, std::vector<Ring<double>>{holeA, holeB})};
    Polygon<double> inHoleB(Ring<double>(std::vector<Vec2DReal>{{10.5,10.5},{10.5,11.5},{11.5,11.5},{11.5,10.5}}));
    check(twoHoles, inHoleB);
}

// ============================================================================
// Base class behavior
// ============================================================================

TEST_F(OGRPolygonAdapterTest, equality) {
    EXPECT_EQ(*withHole, *withHole);
    EXPECT_EQ(*withHole, OGRPolygonAdapter(referenceWithHole()));
    EXPECT_NE(*withHole, *simple);
}

TEST_F(OGRPolygonAdapterTest, equalityIsIndependentOfStartingVertex) {
    Ring<double> rotatedOuter(std::vector<Vec2DReal>{{10,10},{10,0},{0,0},{0,10}});
    OGRPolygonAdapter rotated{Polygon<double>(rotatedOuter, std::vector<Ring<double>>{holeRing()})};
    EXPECT_EQ(*withHole, rotated);
}

TEST_F(OGRPolygonAdapterTest, hash) {
    // the hash is taken over the WKB of the canonical form, so it distinguishes polygons which
    // differ geometrically ...
    EXPECT_NE(simple->hash(), withHole->hash());
    // ... and agrees for polygons which do not
    EXPECT_EQ(withHole->hash(), OGRPolygonAdapter(referenceWithHole()).hash());
}

TEST_F(OGRPolygonAdapterTest, equalPolygonsHashEqually) {
    OGRPolygonAdapter copy(*withHole);
    EXPECT_EQ(withHole->hash(), copy.hash());

    Ring<double> rotatedOuter(std::vector<Vec2DReal>{{10,10},{10,0},{0,0},{0,10}});
    OGRPolygonAdapter rotated{Polygon<double>(rotatedOuter, std::vector<Ring<double>>{holeRing()})};
    EXPECT_EQ(*withHole, rotated);
    EXPECT_EQ(withHole->hash(), rotated.hash());
}

TEST_F(OGRPolygonAdapterTest, toString) {
    auto str = withHole->toString();
    EXPECT_FALSE(str.empty());
    EXPECT_NE(str.find("POLYGON"), std::string::npos);
}
