#include "CLI/CLI.hpp"
#include "fishnet/GDALInitializer.hpp"
#include <filesystem>
#include <future>
#include <CLI/CLI.hpp>
#include <fishnet/Fishnet.hpp>
#include <fishnet/Task.hpp>

class ShapefileMerger: public Task {
public:
    ShapefileMerger():Task("ShapefileMerger"){}

    void operator()(const fishnet::util::range_of<std::filesystem::path> auto& inputs, std::filesystem::path && outputPath){
        fishnet::GDALInitializer::init();
        for(const auto & input: inputs){
            addInput(input);
        }
        addOutput(outputPath);
        std::vector<std::future<fishnet::VectorLayer<fishnet::geometry::OGRGeometryAdapter>>> futures;
        for(size_t i = 1; i < inputs.size();i++){
            futures.push_back(std::async(std::launch::async,[&inputs,i](){return fishnet::VectorIO::read(inputs[i]);}));
        }
        auto firstLayer = fishnet::VectorIO::read(inputs.front());
        auto outputLayer = fishnet::VectorIO::emptyCopy(firstLayer);
        for(auto && feature: firstLayer.getFeatures()){
            outputLayer.addFeature(std::move(feature));
        }
        for(auto & future:futures){
            auto layer = future.get();
            layer.copyFields(outputLayer);
            for(auto && feature: layer.getFeatures()){
                outputLayer.addFeature(std::move(feature));
            }
        }
        addMetric("outputFeatures", fishnet::util::size(outputLayer.getFeatures()));
        fishnet::VectorIO::overwrite(outputLayer, outputPath);
    }
};

int main(int argc, char * argv[]){
    CLI::App app {"FishnetShapefileMerger"};
    std::vector<std::string> inputFilenames;
    std::string outputFilename;
    app.add_option("-i,--inputs",inputFilenames,"Input GIS vector files")->required()->each(CLI::ExistingFile);
    app.add_option("-o,--output",outputFilename,"Output file location")->required()->check([](const std::string & str){
        try{
            auto file = fishnet::getGISFileType(std::filesystem::path(str)).filter([](const auto & format){return std::ranges::contains(fishnet::SUPPORTED_VECTOR_FORMATS,format);});
            if(not file)
                throw std::invalid_argument("Output file type is not supported: "+ str);
            std::filesystem::path parentPath = std::filesystem::path(str).parent_path();
            if(not std::filesystem::exists(parentPath)){
                std::filesystem::create_directories(parentPath);
            }
            return std::string();
        }catch(std::filesystem::filesystem_error & fsError){
            return std::string("Filesystem error when checking/creating output path:\n"+str+"\n")+ fsError.what();
        }
    });
    CLI11_PARSE(app,argc,argv);
    if(inputFilenames.empty()){
        throw std::runtime_error("No input files provided");
    }
    ShapefileMerger()(inputFilenames, std::filesystem::path{outputFilename});
    return 0;
}