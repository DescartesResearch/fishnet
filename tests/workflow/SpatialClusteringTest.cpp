#include <fishnet/TestUtil.hpp>
#include <fishnet/Polygon.hpp>
#include <fishnet/VectorIO.hpp>
#include <fishnet/PathHelper.h>
#include <fishnet/Graph.hpp>

// Tested classes
#include <fishnet/SettlementShape.hpp>
#include <fishnet/DistanceFunction.hpp>
#include <fishnet/DistancePredicate.hpp>
#include <fishnet/IDReduceFunction.hpp>
#include <fishnet/BFSClustering.hpp>
#include <fishnet/DBScan.hpp>


const static std::filesystem::path testFile = fishnet::util::PathHelper::projectDirectory() / "data" / "testing" / "regions" / "Corvara_Small_Preprocessed.shp";
using geometry_type = fishnet::geometry::OGRPolygonAdapter;
using node_type = SettlementShape<geometry_type>;

struct CountingFileRefMapper {
    size_t count = 0;
    FileReference operator()(const auto & file) {
        return FileReference{count++};
    }
};

template<fishnet::geometry::Geometry G>
class ObservableShapefileReader {
public:
    using geometry_type = G;
    using file_type = fishnet::Shapefile;

private:
    OGRSpatialReference spatialRef;
public:
    fishnet::Either<fishnet::VectorLayer<G>,std::string> operator()(const fishnet::Shapefile & shapefile) {
        auto layer = fishnet::VectorIO::tryRead(fishnet::ShapefileReader<G>{},shapefile);
        if(layer)
            this->spatialRef = layer.value().getSpatialReference();
        return layer;
    }

    OGRSpatialReference getSpatialReference() const noexcept {
        return spatialRef;
    }
};

static auto completeGraph(std::ranges::forward_range auto const & nodes) {
    auto graph = fishnet::graph::UndirectedGraph<std::ranges::range_value_t<decltype(nodes)>>(nodes);
    for (auto && from : nodes) {
        for (auto && to : nodes) {
            if (from != to) {
                graph.addEdge(from, to);
            }
        }
    }
    return graph;
}


class SpatialClusteringTest: public ::testing::Test{
protected:
    static void SetUpTestSuite() {
        auto reader = ObservableShapefileReader<geometry_type>{};
        settlements = fishnet::util::toVector(node_type::read(fishnet::Shapefile(testFile),reader,CountingFileRefMapper{}) | std::views::take(50));
        graph = completeGraph(settlements);
        spatialRef = reader.getSpatialReference();
    }
    static inline std::vector<node_type> settlements;
    static inline fishnet::graph::UndirectedGraph<node_type> graph;
    static inline OGRSpatialReference spatialRef = OGRSpatialReference{};
};

TEST_F(SpatialClusteringTest, BFSClustering){
    auto distancePredicate = DistanceBiPredicate{distanceFunctionForSpatialReference(spatialRef), 200.0};
    auto clustering = fishnet::BFSClustering<node_type>{distancePredicate};
    auto result = clustering(graph);
    EXPECT_TRUE(result.clusters.size() > 0);
    fishnet::test::EXPECT_EMPTY(result.noise);
    std::vector<fishnet::geometry::MultiPolygon<geometry_type>> clusters;
    auto reduceFunction = IDReduceFunction{FileReference{42}};
    for(auto && cluster: result.clusters){
        clusters.push_back(reduceFunction(cluster));
    }
}

TEST_F(SpatialClusteringTest, DBSCANClustering){
    auto pointDistanceFunction = distanceFunctionForSpatialReference(spatialRef);
    auto distanceFunction = [&pointDistanceFunction](const node_type & lhs, const node_type & rhs){
        return fishnet::geometry::shapeDistance(lhs.geometry(), rhs.geometry(), pointDistanceFunction);
    };
    auto clustering = fishnet::DBSCAN<node_type>{200.0, 2,distanceFunction};
    auto result = clustering(graph);
    EXPECT_TRUE(result.clusters.size() > 0);
    std::vector<fishnet::geometry::MultiPolygon<geometry_type>> clusters;
    auto reduceFunction = IDReduceFunction{FileReference{42}};
    for(auto && cluster: result.clusters){
        clusters.push_back(reduceFunction(cluster));
    }
}

// projected coordinates (UTM northings ~1e7) round to ~1e-9, above EPSILON, so they are rejected for now
TEST(DistanceFunctionTest, rejectsProjectedCoordinateSystems) {
    OGRSpatialReference utm;
    ASSERT_EQ(utm.importFromEPSG(32737), OGRERR_NONE);   // WGS 84 / UTM zone 37S (Dar es Salaam)
    EXPECT_THROW(distanceFunctionForSpatialReference(utm), std::runtime_error);
    OGRSpatialReference wgs84;
    ASSERT_EQ(wgs84.importFromEPSG(4326), OGRERR_NONE);
    EXPECT_NO_THROW(distanceFunctionForSpatialReference(wgs84));
}
