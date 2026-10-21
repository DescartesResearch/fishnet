# Fishnet
*Framework for Graph-Based Analysis of GIS Vector Data*  
#### Table of Contents
- [Fishnet](#fishnet)
      - [Table of Contents](#table-of-contents)
- [Fishnet Library](#fishnet-library)
  - [Modules](#modules)
  - [Dependencies](#dependencies)
  - [Usage](#usage)
- [Fishnet Binaries](#fishnet-binaries)
  - [Installation](#installation)
  - [CMake Options](#cmake-options)
- [Workflows](#workflows)

# Fishnet Library
The core library contains generic components for graph modeling and traversal, geometry models and sweep line procedures, IO for vector-based GIS files, spatial clustering algorithms and workflow modelling utility.

## Modules
The Fishnet library is organized into the following modules:

| Module | Namespace | Description |
|--------|-----------|-------------|
| **Graph** | `fishnet::graph` | Graph data structures and algorithms including BFS, DFS, connected components, centrality measures (degree, betweenness), graph contraction, and neighborhood search. Supports directed, undirected, and acyclic graphs. |
| **Geometry** | `fishnet::geometry` | Geometric primitives (polygons, lines, points, rings, rectangles), OGR/GDAL-backed adapters for the same primitives, and spatial algorithms including sweep-line, polygon intersection, buffering, and distance computations. |
| **I/O** | `fishnet` | File I/O abstractions, GDAL integration for reading/writing GIS vector data (Shapefile, GeoPackage), vector layer management, and filesystem utilities. |
| **Clustering** | `fishnet` | Spatial clustering algorithms for grouping geometric features based on proximity and other criteria. |
| **Collections** | `fishnet::util` | Custom container types and data structures extending the C++ standard library for framework-specific use cases. |
| **Concepts** | `fishnet::util` | C++20 concepts defining type constraints and requirements used throughout the framework. |
| **Functional** | `fishnet::util`, `fishnet` | Functional programming utilities including function concepts, composite predicates, Option and Either. |
| **Math** | `fishnet::math` | Mathematical utilities including angular types (degrees/radians), numerical operations, and constants. |
| **Workflow** | `::` | Reusable components for composing spatial processing tasks|

## Dependencies
### System Dependencies
Fishnet requires a C++23 compiler (e.g. GCC 13+) and depends on [**GDAL**](https://gdal.org/en/stable/) to provide GIS IO functionality.
GDAL must be installed on the system to build / run fishnet applications. On Ubuntu-based system this can be achieved using the following command:
```shell
sudo apt-get install -y libgdal-dev
``` 
### Project Dependencies
| Name | Repository | Purpose |
| --- | ---| ---|
| CLI11 | https://github.com/CLIUtils/CLI11 | Command line parameter parsing
| JSON | https://github.com/nlohmann/json | JSON C++ library
| magic_enum | https://github.com/Neargye/magic_enum | String <> Enum conversions
| spdlog | https://github.com/gabime/spdlog | Logging
| gtest | https://github.com/google/googletest.git | Testing

## Usage
The following example shows how to store polygons, obtained from a Shapefile, in a graph. Thereafter, the degree centrality measures is calculated on the graph and the results stored as features in the output shapefile.
```cpp
#include <fishnet/Fishnet.hpp>

using namespace fishnet;

int main() {
    using G = geometry::OGRPolygonAdapter;
    Shapefile input {"/path/to/file.shp"};
    auto inputLayer = VectorIO::read<G>(input);
    auto polygons = inputLayer.getGeometries();
    // scale aaBB of polygon by this factor; intersecting buffers -> adjacent
    double bufferMultiplier = 2; 
    size_t maximumNumberOfNeighbours = 5;
    auto adjacencies = geometry::PolygonNeighbours::sweepWithFixedBuffer(polygons,bufferMultiplier,maximumNumberOfNeighbours);
    auto polygonGraph = graph::GraphFactory::UndirectedGraph<G>();
    polygonGraph.addEdges(adjacencies);
    // copy spatial reference from input layer
    auto resultLayer = VectorLayer<G>(inputLayer.getSpatialReference());
    auto degreeCentralityField = resultLayer.addSizeField("degCent").value_or_throw();
    for(const G & polygon: polygonGraph.getNodes()){
        Feature<G> feature {polygon};
        auto degreeCentrality = fishnet::util::size(polygonGraph.getNeighbours(polygon));
        feature.addAttribute(degreeCentralityField,degreeCentrality);
        resultLayer.addFeature(std::move(feature));
    }   
    Shapefile output = input;
    output.appendToFilename("_degree_centrality") ;
    VectorIO::overwrite(resultLayer, output);
}
```
To link the *Fishnet* framework to the program the following *CMake* file can be used:
```cmake 
add_executable(polygonGraph PolygonGraph.cpp)
target_link_libraries(polygonGraph PRIVATE Fishnet::Fishnet)
```


# Fishnet Binaries
Fishnet also provides some common command line applications for processing vector files, found under the [src](src/) directory, plus additional example applications under [example](example/).

| Binary | Description |
| --- | --- |
| **FishnetVectorFileSplitter** | Splits a vector file into a quadtree of tiles (`-d/--depth` levels deep, `4^depth` partitions), to parallelize downstream processing over large datasets. |
| **FishnetVectorFilePreprocessor** | Filters the features of a vector file according to a JSON-described set of predicates (e.g. `ApproxAreaFilter`, `InsidePolygonFilter`), or passes them through unchanged with `--no-filter`. |
| **FishnetVectorFileMerger** | Merges multiple vector files back into a single output file. |

Run any binary with `--help` for its full set of options.

## Installation
Before the binaries can be build using *cmake*, the **GDAL** library has to be installed on the machine. Then the installer script can be invoked as follows:
```shell
./install.sh
```
Alternatively, you can install the binaries as follows:
```shell
mkdir build
cd build
cmake ..
cmake --build . <add custom cmake parameters here>
```
### Docker
Pre-built images are also published to the GitHub Container Registry, see [docker/](docker/) for the Dockerfiles and build script:
| Image | Purpose |
| --- | --- |
| [`ghcr.io/descartesresearch/fishnet-deps`](https://github.com/DescartesResearch/fishnet/pkgs/container/fishnet-deps) | Build-time dependencies (GDAL, compiler toolchain) |
| [`ghcr.io/descartesresearch/fishnet-apps`](https://github.com/DescartesResearch/fishnet/pkgs/container/fishnet-apps) | The Fishnet binaries, built and ready to run |

## CMake Options
| Name | Description  | Default
| --- | --- | --- |
FISHNET_APPS | Build common fishnet applications|  ON
FISHNET_EXAMPLES | Build example applications|  ON
FISHNET_TEST | Enable unit tests for Fishnet components|  ON
FISHNET_TEST_LIB | Build Fishnet test utility library|  ON
FISHNET_COVERAGE | Enable coverage reporting |  OFF
FISHNET_COMPILE_TIME_TRACE | Enable compile time tracing|  OFF
FISHNET_DOCS | Enable documentation generation | OFF

# Workflows
Fishnet is already used in existing production workflows.
| Name | Repository |
|---|---|
| **Settlement Delineation and Analysis (SDA)** | https://github.com/dfg-sos/sda-workflow
| **Africapolis** | https://github.com/LoloGruber/africapolis






