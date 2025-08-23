#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "config.hpp"
#include "utils/Logger.hpp"
#include "utils/Metrics.hpp"

using namespace mev;
using namespace testing;

class ConfigTest : public Test {
protected:
    void SetUp() override {
        // Create a temporary config for testing
        config_manager_.load_default_config();
    }
    
    ConfigManager config_manager_;
};

TEST_F(ConfigTest, DefaultConfigLoading) {
    // Test that default config loads successfully
    EXPECT_NO_THROW(config_manager_.load_default_config());
    
    // Test that required fields are present
    EXPECT_FALSE(config_manager_.get_blockchain_config().ethereum.rpc_url.empty());
    EXPECT_GT(config_manager_.get_performance_config().thread_pool_size, 0);
    EXPECT_GT(config_manager_.get_performance_config().queue_size, 0);
}

TEST_F(ConfigTest, StrategyConfiguration) {
    // Test strategy configuration
    auto strategies = config_manager_.get_strategies_config();
    
    // Check that arbitrage strategy exists and is enabled
    EXPECT_TRUE(strategies.is_strategy_enabled("arbitrage"));
    
    auto arbitrage_config = strategies.get_strategy("arbitrage");
    EXPECT_TRUE(arbitrage_config.enabled);
    EXPECT_GT(arbitrage_config.min_profit_eth, 0.0);
    EXPECT_GT(arbitrage_config.gas_limit, 0);
}

TEST_F(ConfigTest, ConfigurationValidation) {
    // Test configuration validation
    EXPECT_NO_THROW(config_manager_.validate());
    
    // Test that invalid config throws
    ConfigManager invalid_config;
    // This should throw because no config is loaded
    EXPECT_THROW(invalid_config.validate(), std::runtime_error);
}

TEST_F(ConfigTest, JsonSerialization) {
    // Test JSON serialization and deserialization
    auto json = config_manager_.to_json();
    EXPECT_FALSE(json.empty());
    
    // Test that JSON contains expected fields
    EXPECT_TRUE(json.contains("simulation"));
    EXPECT_TRUE(json.contains("strategies"));
    EXPECT_TRUE(json.contains("blockchain"));
    EXPECT_TRUE(json.contains("performance"));
}

class LoggerTest : public Test {
protected:
    void SetUp() override {
        LoggingConfig config;
        config.level = LogLevel::DEBUG;
        config.console_output = true;
        config.file_output = false;
        Logger::initialize(config);
    }
    
    void TearDown() override {
        Logger::shutdown();
    }
};

TEST_F(LoggerTest, BasicLogging) {
    // Test basic logging functionality
    EXPECT_NO_THROW(Logger::info("Test info message"));
    EXPECT_NO_THROW(Logger::warn("Test warning message"));
    EXPECT_NO_THROW(Logger::error("Test error message"));
}

TEST_F(LoggerTest, LogLevels) {
    // Test log level filtering
    Logger::set_level(LogLevel::WARN);
    
    // These should not be logged
    Logger::debug("Debug message");
    Logger::info("Info message");
    
    // These should be logged
    EXPECT_NO_THROW(Logger::warn("Warning message"));
    EXPECT_NO_THROW(Logger::error("Error message"));
}

TEST_F(LoggerTest, StreamLogging) {
    // Test stream-based logging
    LOG_STREAM_INFO << "Stream info message " << 42 << " test";
    LOG_STREAM_DEBUG << "Stream debug message " << 3.14;
}

class MetricsTest : public Test {
protected:
    void SetUp() override {
        MetricsConfig config;
        config.enabled = true;
        config.port = 8081; // Use different port for testing
        Metrics::initialize(config);
    }
    
    void TearDown() override {
        Metrics::shutdown();
    }
};

TEST_F(MetricsTest, CounterMetrics) {
    auto& registry = MetricsRegistry::get_instance();
    auto& counter = registry.create_counter("test_counter", "Test counter");
    
    EXPECT_EQ(counter.get_value(), 0.0);
    
    counter.increment();
    EXPECT_EQ(counter.get_value(), 1.0);
    
    counter.increment(5.0);
    EXPECT_EQ(counter.get_value(), 6.0);
}

TEST_F(MetricsTest, GaugeMetrics) {
    auto& registry = MetricsRegistry::get_instance();
    auto& gauge = registry.create_gauge("test_gauge", "Test gauge");
    
    EXPECT_EQ(gauge.get_value(), 0.0);
    
    gauge.set(10.0);
    EXPECT_EQ(gauge.get_value(), 10.0);
    
    gauge.increment(5.0);
    EXPECT_EQ(gauge.get_value(), 15.0);
    
    gauge.decrement(3.0);
    EXPECT_EQ(gauge.get_value(), 12.0);
}

TEST_F(MetricsTest, HistogramMetrics) {
    auto& registry = MetricsRegistry::get_instance();
    auto& histogram = registry.create_histogram("test_histogram", "Test histogram");
    
    EXPECT_EQ(histogram.get_count(), 0);
    EXPECT_EQ(histogram.get_sum(), 0.0);
    
    histogram.observe(1.0);
    histogram.observe(2.0);
    histogram.observe(3.0);
    
    EXPECT_EQ(histogram.get_count(), 3);
    EXPECT_EQ(histogram.get_sum(), 6.0);
}

TEST_F(MetricsTest, TimerUtility) {
    auto& registry = MetricsRegistry::get_instance();
    auto& histogram = registry.create_histogram("test_timer", "Test timer");
    
    {
        Timer timer(histogram);
        // Simulate some work
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    } // Timer automatically stops here
    
    EXPECT_EQ(histogram.get_count(), 1);
    EXPECT_GT(histogram.get_sum(), 0.0);
}

TEST_F(MetricsTest, PrometheusExport) {
    auto& registry = MetricsRegistry::get_instance();
    
    // Create some test metrics
    auto& counter = registry.create_counter("test_prometheus_counter", "Test counter for Prometheus");
    auto& gauge = registry.create_gauge("test_prometheus_gauge", "Test gauge for Prometheus");
    
    counter.increment(5.0);
    gauge.set(10.0);
    
    // Export to Prometheus format
    std::string prometheus = registry.to_prometheus();
    
    // Check that metrics are present in output
    EXPECT_THAT(prometheus, HasSubstr("test_prometheus_counter"));
    EXPECT_THAT(prometheus, HasSubstr("test_prometheus_gauge"));
    EXPECT_THAT(prometheus, HasSubstr("5"));
    EXPECT_THAT(prometheus, HasSubstr("10"));
}

TEST_F(MetricsTest, JsonExport) {
    auto& registry = MetricsRegistry::get_instance();
    
    // Create some test metrics
    auto& counter = registry.create_counter("test_json_counter", "Test counter for JSON");
    auto& gauge = registry.create_gauge("test_json_gauge", "Test gauge for JSON");
    
    counter.increment(3.0);
    gauge.set(7.0);
    
    // Export to JSON format
    auto json = registry.to_json();
    
    // Check that metrics are present in output
    EXPECT_TRUE(json.contains("test_json_counter"));
    EXPECT_TRUE(json.contains("test_json_gauge"));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::InitGoogleMock(&argc, argv);
    
    return RUN_ALL_TESTS();
}
