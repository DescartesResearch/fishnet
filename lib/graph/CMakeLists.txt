add_subdirectory(adjacency)
add_subdirectory(model)
add_subdirectory(traversal)
add_subdirectory(measures)
SET(GraphLib fishnet_graph)
add_library(${GraphLib} INTERFACE)
target_link_libraries(${GraphLib} INTERFACE 
    ${FISHNET}::Graph::Adjacency
    ${FISHNET}::Graph::Algorithm
    ${FISHNET}::Graph::Model
    ${FISHNET}::Graph::Centrality
    util
)
target_include_directories(${GraphLib} INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
add_library(${FISHNET}::Graph ALIAS ${GraphLib})