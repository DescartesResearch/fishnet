add_subdirectory(graphTestUtils)

set(GraphTestBin Test-Graph)
add_executable(${GraphTestBin} 
CentralityTest.cpp 
UndirectedGraphTest.cpp 
DirectedGraphTest.cpp 
ConnectedComponentsTest.cpp 
SearchPathTest.cpp 
ContractionTest.cpp 
LabelledGraphTest.cpp
LabelledEdgeTest.cpp
EdgeTest.cpp
DAGTest.cpp
JSONAdjacencyTest.cpp
BinaryAdjacencyTest.cpp
NeighborhoodTest.cpp
MSTAlgorithmTest.cpp
SubgraphsTest.cpp
)
gtest_discover_tests(${GraphTestBin})
target_link_libraries(${GraphTestBin} PRIVATE Fishnet::Test ${FISHNET}::Graph graphTest_util ${FISHNET}::IO::Console)