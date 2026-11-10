#pragma once
#include <fishnet/StopWatch.h>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <vector>

/**
 * @brief Common super class for all tasks
 * Execution time is tracked, Brings logger into scope
 * RAII type -> execution time is printed on destruction
 * Recorded parameters, input/output files and metrics are logged as a JSON summary on destruction
 */
class Task {
private:
    using Json = nlohmann::ordered_json;
    std::string taskName;
    fishnet::util::StopWatch stopWatch;
    int uncaughtExceptions = std::uncaught_exceptions();
    Json parameters = Json::object();
    Json metrics = Json::object();
    std::vector<std::filesystem::path> inputs;
    std::vector<std::filesystem::path> outputs;

    static Json describeFiles(const std::vector<std::filesystem::path> & files) {
        auto description = Json::array();
        for(const auto & file: files){
            std::error_code error;
            auto bytes = std::filesystem::file_size(file, error);
            description.push_back({{"path", file.string()}, {"bytes", error ? Json() : Json(bytes)}});
        }
        return description;
    }

    void logSummary(double duration) const {
        Json summary = {
            {"task", taskName},
            {"status", std::uncaught_exceptions() > uncaughtExceptions ? "failed" : "success"},
            {"seconds", duration},
            {"parameters", parameters},
            {"inputs", describeFiles(inputs)},
            {"outputs", describeFiles(outputs)},
            {"metrics", metrics}
        };
        if(workflowID != 0)
            summary["workflowID"] = workflowID;
        spdlog::info("Summary: {}", summary.dump(-1, ' ', false, Json::error_handler_t::replace));
    }

protected:
    size_t workflowID = 0;

    void addParameter(const std::string & name, Json value) { parameters[name] = std::move(value); }
    void addMetric(const std::string & name, Json value) { metrics[name] = std::move(value); }
    void addInput(std::filesystem::path path) { inputs.push_back(std::move(path)); }
    void addOutput(std::filesystem::path path) { outputs.push_back(std::move(path)); }

public:
    Task(std::string taskName):taskName(std::move(taskName)){
        spdlog::info("Launching {}",this->taskName);
    }

    Task(std::string taskName, size_t workflowID):taskName(std::move(taskName)), workflowID(workflowID){
        spdlog::info("Launching {} with workflow id {}",this->taskName,workflowID);
    }

    constexpr static std::string FISHNET_ID_FIELD = "FISHNET_ID";

    virtual ~Task(){
        double duration =stopWatch.stop();
        spdlog::info("Completed {}", this->taskName);
        spdlog::info("Time elapsed in seconds:{}",duration);
        try {
            logSummary(duration);
        } catch(const std::exception & e) {
            spdlog::warn("Could not log the summary of {}: {}", this->taskName, e.what());
        }
    }
};
