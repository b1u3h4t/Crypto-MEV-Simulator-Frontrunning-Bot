#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>

namespace mev {

// Forward declarations
class ConfigManager;

// Simulation modes
enum class SimulationMode {
    REALTIME,
    HISTORICAL,
    SYNTHETIC
};

// Strategy types
enum class StrategyType {
    ARBITRAGE,
    SANDWICH,
    FRONTRUN,
    LIQUIDATION
};

// CLI configuration structure
struct SimulationConfig {
    SimulationMode mode = SimulationMode::REALTIME;
    std::string config_file;
    std::vector<std::string> enabled_strategies;
    uint64_t start_block = 0;
    uint64_t block_count = 0;
    uint64_t duration_seconds = 0;
    uint64_t tx_rate = 0;
    bool enable_visualization = false;
    bool enable_profiling = false;
    std::vector<std::string> export_formats;
    std::string fork_url;
    uint64_t fork_block = 0;
};

// Strategy configuration
struct StrategyConfig {
    bool enabled = false;
    double min_profit_eth = 0.01;
    double max_slippage_percent = 0.5;
    std::vector<std::string> target_dexes;
    uint64_t gas_limit = 500000;
    uint64_t max_gas_price_gwei = 100;
    uint64_t bundle_timeout_ms = 1000;
    double frontrun_gas_multiplier = 1.1;
    double backrun_gas_multiplier = 1.05;
    double priority_fee_gwei = 2.0;
    double min_transaction_value_eth = 0.1;
    std::vector<std::string> target_protocols;
};

// Simulation configuration
struct SimulationSettings {
    SimulationMode mode = SimulationMode::REALTIME;
    bool mempool_emulation = true;
    bool block_simulation = true;
    bool historical_replay = false;
    struct {
        bool enabled = false;
        uint64_t transaction_rate = 1000;
        uint64_t duration_seconds = 3600;
    } synthetic_data;
};

// Performance configuration
struct PerformanceConfig {
    uint32_t thread_pool_size = 16;
    uint32_t queue_size = 10000;
    uint32_t latency_target_us = 100;
    uint32_t max_concurrent_requests = 100;
    bool enable_simd = true;
    bool memory_mapping = true;
};

// Blockchain configuration
struct BlockchainConfig {
    struct {
        std::string rpc_url;
        uint32_t chain_id = 1;
        uint32_t block_time_seconds = 12;
    } ethereum;
    
    struct {
        std::string relay_url;
        uint64_t bundle_timeout_ms = 1000;
        uint32_t max_bundle_size = 10;
    } flashbots;
    
    struct {
        bool enabled = false;
        std::string url;
        uint64_t block_number = 0;
    } fork;
};

// Trading configuration
struct TradingConfig {
    struct {
        bool enabled = true;
        double base_fee_multiplier = 1.1;
        std::string priority_fee_strategy = "dynamic";
        uint64_t max_gas_price_gwei = 100;
    } gas_optimization;
    
    struct {
        double default_percent = 0.5;
        double max_percent = 2.0;
        bool dynamic_adjustment = true;
    } slippage;
    
    struct {
        uint32_t max_transactions = 10;
        uint64_t timeout_ms = 1000;
        uint32_t retry_attempts = 3;
    } bundle;
};

// Monitoring configuration
struct MonitoringConfig {
    struct {
        bool enabled = true;
        uint16_t port = 8080;
        uint32_t export_interval_seconds = 60;
    } metrics;
    
    struct {
        std::string level = "INFO";
        std::string file = "logs/mev_sim.log";
        uint32_t max_file_size_mb = 100;
        uint32_t max_files = 10;
    } logging;
    
    struct {
        bool enabled = true;
        uint32_t update_interval_ms = 1000;
        std::vector<std::string> export_formats = {"csv", "json"};
    } visualization;
};

// Data configuration
struct DataConfig {
    struct {
        std::string type = "file";
        std::string directory = "./data";
        bool compression = true;
    } storage;
    
    struct {
        bool enabled = true;
        uint32_t ttl_seconds = 300;
        uint32_t max_size_mb = 1000;
    } cache;
    
    struct {
        std::vector<std::string> formats = {"csv", "json"};
        bool include_metrics = true;
        std::string timestamp_format = "iso8601";
    } export_settings;
};

// Security configuration
struct SecurityConfig {
    struct {
        bool enabled = true;
        uint32_t requests_per_second = 100;
        uint32_t burst_size = 50;
    } rate_limiting;
    
    struct {
        bool enable_ssl_verification = true;
        uint32_t max_request_size_mb = 10;
        uint32_t timeout_ms = 5000;
    } validation;
};

// Strategies configuration
struct StrategiesConfig {
    std::map<std::string, StrategyConfig> strategies;
    
    StrategyConfig& get_strategy(const std::string& name) {
        return strategies[name];
    }
    
    const StrategyConfig& get_strategy(const std::string& name) const {
        auto it = strategies.find(name);
        if (it == strategies.end()) {
            throw std::runtime_error("Strategy not found: " + name);
        }
        return it->second;
    }
    
    bool is_strategy_enabled(const std::string& name) const {
        auto it = strategies.find(name);
        return it != strategies.end() && it->second.enabled;
    }
};

// Main configuration structure
struct Config {
    SimulationSettings simulation;
    PerformanceConfig performance;
    StrategiesConfig strategies;
    BlockchainConfig blockchain;
    TradingConfig trading;
    MonitoringConfig monitoring;
    DataConfig data;
    SecurityConfig security;
};

// Configuration manager class
class ConfigManager {
public:
    ConfigManager();
    ~ConfigManager() = default;
    
    // Load configuration from file
    void load_config(const std::string& file_path);
    
    // Load default configuration
    void load_default_config();
    
    // Override configuration with CLI arguments
    void override_config(const SimulationConfig& cli_config);
    
    // Get configuration sections
    const SimulationSettings& get_simulation_config() const { return config.simulation; }
    const PerformanceConfig& get_performance_config() const { return config.performance; }
    const StrategiesConfig& get_strategies_config() const { return config.strategies; }
    const BlockchainConfig& get_blockchain_config() const { return config.blockchain; }
    const TradingConfig& get_trading_config() const { return config.trading; }
    const MonitoringConfig& get_monitoring_config() const { return config.monitoring; }
    const DataConfig& get_data_config() const { return config.data; }
    const SecurityConfig& get_security_config() const { return config.security; }
    
    // Get specific strategy configuration
    const StrategyConfig& get_strategy_config(const std::string& strategy_name) const;
    
    // Check if strategy is enabled
    bool is_strategy_enabled(const std::string& strategy_name) const;
    
    // Get all enabled strategies
    std::vector<std::string> get_enabled_strategies() const;
    
    // Validate configuration
    void validate() const;
    
    // Export configuration to JSON
    nlohmann::json to_json() const;
    
    // Import configuration from JSON
    void from_json(const nlohmann::json& j);

private:
    Config config;
    
    // Helper methods for parsing JSON
    void parse_simulation_config(const nlohmann::json& j);
    void parse_performance_config(const nlohmann::json& j);
    void parse_strategies_config(const nlohmann::json& j);
    void parse_blockchain_config(const nlohmann::json& j);
    void parse_trading_config(const nlohmann::json& j);
    void parse_monitoring_config(const nlohmann::json& j);
    void parse_data_config(const nlohmann::json& j);
    void parse_security_config(const nlohmann::json& j);
    
    // Helper methods for JSON serialization
    nlohmann::json simulation_to_json() const;
    nlohmann::json performance_to_json() const;
    nlohmann::json strategies_to_json() const;
    nlohmann::json blockchain_to_json() const;
    nlohmann::json trading_to_json() const;
    nlohmann::json monitoring_to_json() const;
    nlohmann::json data_to_json() const;
    nlohmann::json security_to_json() const;
};

} // namespace mev
