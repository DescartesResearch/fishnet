#include <gtest/gtest.h>
#include <spdlog/sinks/ostream_sink.h>
#include <fstream>
#include <sstream>
#include <stdexcept>

// Tested class
#include <fishnet/Task.hpp>

struct TestTask: public Task {
    TestTask():Task("TestTask"){}
    using Task::addParameter;
    using Task::addMetric;
    using Task::addInput;
    using Task::addOutput;
};

class TaskTest: public ::testing::Test {
protected:
    std::ostringstream log;
    std::shared_ptr<spdlog::logger> previousLogger;
    std::filesystem::path directory = std::filesystem::path(::testing::TempDir()) / "fishnet_task_test";

    void SetUp() override {
        previousLogger = spdlog::default_logger();
        auto logger = std::make_shared<spdlog::logger>("test", std::make_shared<spdlog::sinks::ostream_sink_st>(log));
        logger->set_pattern("%v");
        spdlog::set_default_logger(logger);
        std::filesystem::create_directories(directory);
    }

    void TearDown() override {
        spdlog::set_default_logger(previousLogger);
        std::filesystem::remove_all(directory);
    }

    nlohmann::json summary() const {
        constexpr std::string_view prefix = "Summary: ";
        std::istringstream lines(log.str());
        for(std::string line; std::getline(lines, line);){
            if(line.starts_with(prefix))
                return nlohmann::json::parse(line.substr(prefix.size()));
        }
        throw std::runtime_error("No summary logged");
    }
};

TEST_F(TaskTest, logsSummary) {
    auto input = directory / "input.txt";
    auto output = directory / "output.txt";
    auto missing = directory / "missing.txt";
    std::ofstream(input) << "12345";
    {
        TestTask task;
        task.addParameter("depth", 3);
        task.addParameter("format", "shp");
        task.addInput(input);
        task.addInput(missing);
        task.addOutput(output);
        task.addMetric("features", 42);
        // outputs are measured on destruction, so they may be written after being recorded
        std::ofstream(output) << "123";
    }
    auto json = summary();
    EXPECT_EQ(json["task"], "TestTask");
    EXPECT_EQ(json["status"], "success");
    EXPECT_GE(json["seconds"].get<double>(), 0.0);
    EXPECT_EQ(json["parameters"], nlohmann::json({{"depth", 3}, {"format", "shp"}}));
    EXPECT_EQ(json["metrics"], nlohmann::json({{"features", 42}}));
    ASSERT_EQ(json["inputs"].size(), 2);
    EXPECT_EQ(json["inputs"][0]["path"], input.string());
    EXPECT_EQ(json["inputs"][0]["bytes"], 5);
    EXPECT_TRUE(json["inputs"][1]["bytes"].is_null());
    ASSERT_EQ(json["outputs"].size(), 1);
    EXPECT_EQ(json["outputs"][0]["bytes"], 3);
    EXPECT_FALSE(json.contains("workflowID"));
}

TEST_F(TaskTest, reportsFailureOnException) {
    EXPECT_THROW({
        TestTask task;
        throw std::runtime_error("failure");
    }, std::runtime_error);
    EXPECT_EQ(summary()["status"], "failed");
}
