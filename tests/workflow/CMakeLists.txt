set(WorkflowTestBin Test-Workflow)
add_executable(${WorkflowTestBin}
PolygonDistanceTest.cpp
SettlementShapeTest.cpp
SpatialClusteringTest.cpp
TaskTest.cpp
)
gtest_discover_tests(${WorkflowTestBin})
target_link_libraries(${WorkflowTestBin} PRIVATE  ${FISHNET}::${FISHNET} ${FISHNET}::Workflow ${FISHNET}::Test geometryTestUtils ) 
