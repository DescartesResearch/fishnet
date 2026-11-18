add_subdirectory(geometryTestUtils)

set(GeometryTestBin Test-Geometry)
add_executable(${GeometryTestBin}
Vec2DTest.cpp
LineTest.cpp
SegmentTest.cpp
PolygonDistanceTest.cpp
RayTest.cpp
RingTest.cpp
SimplePolygonTest.cpp
PolygonTest.cpp
MultiPolygonTest.cpp
kNearestNeighboursTest.cpp
SweepLineTest.cpp
PolygonNeighboursTest.cpp
TriangleTest.cpp
DelaunayTriangulationTest.cpp
OGRRingAdapterTest.cpp
OGRPolygonAdapterTest.cpp
OGRMultiPolygonAdapterTest.cpp
OGRGeometryAdapterTest.cpp
#CharacteristicShapeTest.cpp
)
gtest_discover_tests(${GeometryTestBin})
target_link_libraries(${GeometryTestBin} PRIVATE ${FISHNET}::Geometry ${FISHNET}::Test geometryTestUtils ${FISHNET}::IO)