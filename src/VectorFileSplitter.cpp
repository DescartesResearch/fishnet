#include "fishnet/Option.hpp"
#include <fishnet/Polygon.hpp>
#include <fishnet/VectorIO.hpp>
#include <fishnet/PathHelper.h>
#include <fishnet/GISFactory.hpp>
#include <fishnet/GISFile.hpp>
#include <fishnet/Rectangle.hpp>
#include <fishnet/Task.hpp>
#include <CLI/CLI.hpp>
#include <magic_enum.hpp>

using namespace fishnet;
using GeometryType = fishnet::geometry::OGRPolygonAdapter;

struct VectorFileSplitterConfig {
    uint32_t depth;
    int xOffset = 0;
    int yOffset = 0;
    GISFileType outputFormat;
};

/**
 * @brief Performs a quadtree-based splitting of a shapefile. For example with a depth of 1, the input is split into 4 quadrants
 * 
 */
class VectorFileSplitter: public Task {
public:
    VectorFileSplitter():Task("VectorFileSplitter"){}

    static void operator()(const AbstractVectorFile & source, const std::filesystem::path & outputDirectory, const VectorFileSplitterConfig & config ){
        spdlog::set_level(spdlog::level::debug);
        const auto input = VectorIO::read<GeometryType>(source);
        const auto boundingBox = geometry::minimalBoundingBox(input.getGeometries());
        double deltaX = boundingBox.right()-boundingBox.left();
        double deltaY = boundingBox.top()-boundingBox.bottom();
        std::vector<VectorLayer<GeometryType>> outputDatasets;

        size_t chunksPerDimension = static_cast<size_t>(1 << config.depth);
        outputDatasets.reserve(chunksPerDimension*chunksPerDimension);
        spdlog::debug("Depth {}, ChunksPerDimension {}, Total Partitions {}",config.depth,chunksPerDimension,chunksPerDimension*chunksPerDimension);
        for(size_t y = 0; y < chunksPerDimension ; y++){
            for(size_t x = 0; x < chunksPerDimension; x++){
                outputDatasets.push_back(VectorIO::emptyCopy<GeometryType>(input));
            }
        }
        for(auto && feature: input.getFeatures()){
            auto centroid = feature.getGeometry().centroid();
            // clamp into the grid: a centroid on the right or top edge of the bounding box maps to chunksPerDimension
            auto chunkIndex = [chunksPerDimension](double offset, double delta){
                return static_cast<size_t>(std::clamp(offset / delta * static_cast<double>(chunksPerDimension), 0.0, static_cast<double>(chunksPerDimension - 1)));
            };
            size_t x = chunkIndex(centroid.x - boundingBox.left(), deltaX);
            size_t y = chunkIndex(centroid.y - boundingBox.bottom(), deltaY);
            outputDatasets.at(y*chunksPerDimension+x).addFeature(std::move(feature));
        }
        
        for(size_t y = 0; y < chunksPerDimension ; y++){
            for(size_t x = 0; x < chunksPerDimension; x++){
                const auto & ds = outputDatasets.at(y*chunksPerDimension+x);
                if(util::size(ds.getFeatures()) > 0) {
                    std::filesystem::path dest = ( outputDirectory / fishnet::util::PathHelper::appendToFilename(source.getPath().stem().string() + getExtension(config.outputFormat),"_"+ std::to_string(static_cast<int64_t>(x)+config.xOffset)+"_"+std::to_string(static_cast<int64_t>(y)+config.yOffset)).filename());
                    VectorIO::overwrite(ds, dest);
                }
            }
        }
    }
};

int main(int argc, char * argv[]) {
    CLI::App app  {"Fishnet Vector File Splitter"};
    std::string inputFilename;
    std::string outputDirectoryName;
    uint32_t depth;
    int xOffset=0;
    int yOffset=0;
    std::string format;
    app.add_option("-i,--input",inputFilename,"Path to the input file")->required()->check(CLI::ExistingFile);
    app.add_option("-o,--outputDirectory",outputDirectoryName)->check(CLI::ExistingDirectory)->default_val(std::filesystem::current_path());
    app.add_option("-f,--format",format,"Format of the output file")->check([](const std::string & f){
        auto type = fishnet::Option(magic_enum::enum_cast<GISFileType>(f)).filter([](const auto & format){return std::ranges::contains(SUPPORTED_VECTOR_FORMATS,format);});       
        return type.transform([](auto && format){return std::string(); /*OK*/}).value_or(std::string("Unsupported format: "+f+" (supported formats: SHAPEFILE, GEOPACKAGE)"));
    });
    app.add_option("-d,--depth",depth,"Split depth: results in 4^depth partitions")->required();
    app.add_option("-x",xOffset,"x offset for the tile coordinates of the output files");
    app.add_option("-y",yOffset,"y offset for the tile coordinates of the output files");
    CLI11_PARSE(app,argc,argv);
    auto outputFormat = fishnet::Option(magic_enum::enum_cast<GISFileType>(format))
        .or_else([&inputFilename](){return getGISFileType(std::filesystem::path(inputFilename));})
        .value_or_throw("Could not determine output format from input file extension and no valid format was specified");
    spdlog::set_level(spdlog::level::debug);
    spdlog::debug("FormatString: {}, OutputFormat: {}",format,magic_enum::enum_name(outputFormat));
    VectorFileSplitter()(
        {inputFilename},
        {outputDirectoryName},
        VectorFileSplitterConfig{.depth=depth,.xOffset=xOffset,.yOffset=yOffset, .outputFormat=outputFormat}
    );
    return 0;
}