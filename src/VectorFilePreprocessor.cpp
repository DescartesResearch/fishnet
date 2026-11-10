#include <CLI/CLI.hpp>
#include <nlohmann/json.hpp>
#include <fishnet/Fishnet.hpp>
#include <fishnet/Task.hpp>
#include <fishnet/TaskConfig.hpp>
#include <fishnet/JSONFilterFactory.hpp>
#include <fishnet/CompositePredicate.hpp>

struct VectorFilePreprocessorConfig {
    constexpr static const char * FILTER_KEY = "filters";
    nlohmann::json filterJson;
    bool runFilter;

    VectorFilePreprocessorConfig(const nlohmann::json & configJson, bool runFilter){
        this->runFilter = runFilter;
        if(configJson.contains(FILTER_KEY))
            configJson.at(FILTER_KEY).get_to(this->filterJson);
    }
};

template<fishnet::geometry::Geometry G>
class VectorFilePreprocessor : public Task {
private:
    VectorFilePreprocessorConfig config;
    std::filesystem::path inputFile;
    std::filesystem::path outputFile;

    fishnet::util::AllOfPredicate<G> loadFilters() const {
        fishnet::util::AllOfPredicate<G> predicate;
        for(auto && filter: JSONFilterFactory<G>::getFilters(config.filterJson)){
            predicate.add(std::move(filter));
        }
        return predicate;
    }

    fishnet::util::AllOfPredicate<G,G> loadBinaryFilters() const {
        fishnet::util::AllOfPredicate<G,G> biPredicate;
        for(auto && biFilter: JSONFilterFactory<G>::getBinaryFilters(config.filterJson)){
            biPredicate.add(std::move(biFilter));
        }
        return biPredicate;
    }

    void writeOutput(fishnet::util::forward_range_of<fishnet::Feature<G>> auto && features, fishnet::VectorLayer<G> & output) {
        auto idField = output.getSizeField(Task::FISHNET_ID_FIELD).or_else([&output](){return output.addSizeField(Task::FISHNET_ID_FIELD);}).value_or_throw();
        auto geometryHasher = std::hash<G>();
        for(auto && feature: features){
            feature.setAttribute(idField, fishnet::normalizeToShpFileIntField(geometryHasher(feature.getGeometry())));
            output.addFeature(std::move(feature));
        }
        addMetric("outputFeatures", fishnet::util::size(output.getFeatures()));
        fishnet::VectorIO::overwrite(output, this->outputFile);
    }
    
public:
    VectorFilePreprocessor(VectorFilePreprocessorConfig config, std::filesystem::path input, std::filesystem::path output)
        :Task("VectorFilePreprocessor"),config(std::move(config)),inputFile(std::move(input)),outputFile(std::move(output)){
        addInput(this->inputFile);
        addOutput(this->outputFile);
        addParameter("runFilter", this->config.runFilter);
        addParameter("filters", this->config.filterJson);
    }

    void run() {
        auto input = fishnet::VectorIO::read<G>(this->inputFile);
        addMetric("inputFeatures", fishnet::util::size(input.getFeatures()));
        auto output = fishnet::VectorIO::emptyCopy<G>(input);
        if (not this->config.runFilter){
            writeOutput(input.getFeatures(), output);
            return;
        }
        auto filter = loadFilters();
        auto binaryFilter = loadBinaryFilters();
        auto proj = []( const auto & feature){
            return feature.getGeometry();
        };
        auto filteredFeatures = fishnet::geometry::filter(input.getFeatures(),proj,binaryFilter,filter);
        writeOutput(filteredFeatures, output);
    }
};
constexpr static const char * OUTPUT_SUFFIX = "_filtered";

int main(int argc, char * argv[]){
    using ShapeType = fishnet::geometry::OGRPolygonAdapter;
    CLI::App app{"Shapefile Preprocessor"};
    std::string configFilename;
    std::string inputFilename;
    bool noFilter = false;
    app.add_option("-i,--input",inputFilename,"Input vector file for the filter step")->required()->check(CLI::ExistingFile);
    app.add_option("-c,--config", configFilename, "Json description of the preprocessing task")->check(CLI::ExistingFile);
    app.add_flag("--no-filter", noFilter, "Disable filtering of the input file");
    CLI11_PARSE(app, argc, argv);
    auto outputFile = std::filesystem::path(inputFilename).stem().string() + OUTPUT_SUFFIX + std::filesystem::path(inputFilename).extension().string();
    VectorFilePreprocessor<ShapeType>(
        VectorFilePreprocessorConfig {configFilename.empty() ? nlohmann::json() : nlohmann::json::parse(std::ifstream(configFilename)), !noFilter},
        {std::move(inputFilename)},
        {std::move(outputFile)}
    ).run();
    return 0;
}