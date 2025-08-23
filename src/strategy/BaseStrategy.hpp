#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <atomic>
#include <mutex>

#include "config.hpp"
#include "utils/Logger.hpp"
#include "utils/Metrics.hpp"

namespace mev {

// Forward declarations
class Transaction;
class Bundle;
class Opportunity;

// Strategy result
enum class StrategyResult {
    SUCCESS,
    FAILED,
    NO_OPPORTUNITY,
    INSUFFICIENT_PROFIT,
    HIGH_SLIPPAGE,
    GAS_TOO_HIGH,
    TIMEOUT,
    ERROR
};

// Opportunity structure
struct Opportunity {
    std::string id;
    std::string strategy_name;
    double expected_profit_eth;
    double estimated_gas_cost_eth;
    double net_profit_eth;
    double slippage_percent;
    uint64_t gas_limit;
    uint64_t gas_price_gwei;
    std::chrono::system_clock::time_point timestamp;
    std::vector<std::string> target_transactions;
    std::map<std::string, std::string> metadata;
    
    // Validation
    bool is_profitable() const {
        return net_profit_eth > 0.0;
    }
    
    bool is_within_slippage_limit(double max_slippage) const {
        return slippage_percent <= max_slippage;
    }
    
    bool is_within_gas_limit(uint64_t max_gas_price) const {
        return gas_price_gwei <= max_gas_price;
    }
};

// Strategy execution context
struct StrategyContext {
    uint64_t block_number;
    uint64_t block_timestamp;
    double current_gas_price_gwei;
    double base_fee_gwei;
    double priority_fee_gwei;
    std::vector<std::string> mempool_transactions;
    std::map<std::string, double> token_prices;
    std::map<std::string, double> dex_liquidity;
    
    // Performance tracking
    std::chrono::high_resolution_clock::time_point start_time;
    std::chrono::high_resolution_clock::time_point end_time;
};

// Strategy statistics
struct StrategyStats {
    uint64_t opportunities_detected = 0;
    uint64_t opportunities_executed = 0;
    uint64_t successful_executions = 0;
    uint64_t failed_executions = 0;
    double total_profit_eth = 0.0;
    double total_gas_used_eth = 0.0;
    double avg_execution_time_us = 0.0;
    double success_rate = 0.0;
    
    // Latency tracking
    double avg_detection_latency_us = 0.0;
    double avg_execution_latency_us = 0.0;
    double min_execution_latency_us = std::numeric_limits<double>::max();
    double max_execution_latency_us = 0.0;
    
    // Profit tracking
    double min_profit_eth = std::numeric_limits<double>::max();
    double max_profit_eth = 0.0;
    double avg_profit_eth = 0.0;
    
    void update_success(double profit_eth, double gas_used_eth, double execution_time_us) {
        successful_executions++;
        total_profit_eth += profit_eth;
        total_gas_used_eth += gas_used_eth;
        
        avg_execution_time_us = (avg_execution_time_us * (successful_executions - 1) + execution_time_us) / successful_executions;
        avg_profit_eth = total_profit_eth / successful_executions;
        
        min_profit_eth = std::min(min_profit_eth, profit_eth);
        max_profit_eth = std::max(max_profit_eth, profit_eth);
        
        min_execution_latency_us = std::min(min_execution_latency_us, execution_time_us);
        max_execution_latency_us = std::max(max_execution_latency_us, execution_time_us);
        
        success_rate = static_cast<double>(successful_executions) / opportunities_executed;
    }
    
    void update_failure(double execution_time_us) {
        failed_executions++;
        avg_execution_time_us = (avg_execution_time_us * (opportunities_executed - 1) + execution_time_us) / opportunities_executed;
        success_rate = static_cast<double>(successful_executions) / opportunities_executed;
        
        min_execution_latency_us = std::min(min_execution_latency_us, execution_time_us);
        max_execution_latency_us = std::max(max_execution_latency_us, execution_time_us);
    }
    
    void reset() {
        opportunities_detected = 0;
        opportunities_executed = 0;
        successful_executions = 0;
        failed_executions = 0;
        total_profit_eth = 0.0;
        total_gas_used_eth = 0.0;
        avg_execution_time_us = 0.0;
        success_rate = 0.0;
        avg_detection_latency_us = 0.0;
        avg_execution_latency_us = 0.0;
        min_execution_latency_us = std::numeric_limits<double>::max();
        max_execution_latency_us = 0.0;
        min_profit_eth = std::numeric_limits<double>::max();
        max_profit_eth = 0.0;
        avg_profit_eth = 0.0;
    }
};

// Base strategy class
class BaseStrategy {
public:
    explicit BaseStrategy(const std::string& name, const StrategyConfig& config);
    virtual ~BaseStrategy() = default;
    
    // Strategy interface
    virtual bool detect_opportunity(const StrategyContext& context, Opportunity& opportunity) = 0;
    virtual StrategyResult execute_opportunity(const Opportunity& opportunity, Bundle& bundle) = 0;
    
    // Strategy lifecycle
    virtual void initialize();
    virtual void shutdown();
    virtual void reset();
    
    // Configuration
    const std::string& get_name() const { return name_; }
    const StrategyConfig& get_config() const { return config_; }
    void update_config(const StrategyConfig& config);
    
    // Statistics
    const StrategyStats& get_stats() const;
    void reset_stats();
    
    // State
    bool is_enabled() const { return enabled_; }
    void set_enabled(bool enabled) { enabled_ = enabled; }
    
    // Performance monitoring
    void record_detection_latency(double latency_us);
    void record_execution_latency(double latency_us);
    
    // Validation
    virtual bool validate_opportunity(const Opportunity& opportunity) const;
    virtual bool validate_bundle(const Bundle& bundle) const;
    
    // Utility methods
    double calculate_net_profit(const Opportunity& opportunity) const;
    double estimate_gas_cost(uint64_t gas_limit, uint64_t gas_price_gwei) const;
    double calculate_slippage(double expected_price, double actual_price) const;
    
    // Metrics
    void update_metrics(const Opportunity& opportunity, StrategyResult result, double execution_time_us);

protected:
    // Strategy name and configuration
    std::string name_;
    StrategyConfig config_;
    bool enabled_;
    
    // Statistics
    mutable std::mutex stats_mutex_;
    StrategyStats stats_;
    
    // Performance metrics
    std::unique_ptr<Counter> opportunities_detected_counter_;
    std::unique_ptr<Counter> opportunities_executed_counter_;
    std::unique_ptr<Counter> successful_executions_counter_;
    std::unique_ptr<Counter> failed_executions_counter_;
    std::unique_ptr<Gauge> total_profit_gauge_;
    std::unique_ptr<Gauge> total_gas_used_gauge_;
    std::unique_ptr<Histogram> detection_latency_histogram_;
    std::unique_ptr<Histogram> execution_latency_histogram_;
    std::unique_ptr<Histogram> profit_histogram_;
    
    // Internal methods
    virtual void initialize_metrics();
    virtual void log_opportunity(const Opportunity& opportunity);
    virtual void log_execution_result(const Opportunity& opportunity, StrategyResult result);
    
    // Helper methods for subclasses
    bool is_profitable_enough(double profit_eth) const;
    bool is_gas_price_acceptable(uint64_t gas_price_gwei) const;
    bool is_slippage_acceptable(double slippage_percent) const;
    
    // Timing utilities
    std::chrono::high_resolution_clock::time_point get_current_time() const;
    double calculate_latency_us(const std::chrono::high_resolution_clock::time_point& start,
                               const std::chrono::high_resolution_clock::time_point& end) const;
    
    // Logging utilities
    void log_debug(const std::string& message) const;
    void log_info(const std::string& message) const;
    void log_warn(const std::string& message) const;
    void log_error(const std::string& message) const;
};

// Strategy factory
class StrategyFactory {
public:
    using StrategyCreator = std::function<std::unique_ptr<BaseStrategy>(const std::string&, const StrategyConfig&)>;
    
    static StrategyFactory& get_instance();
    
    // Register strategy type
    void register_strategy(const std::string& type, StrategyCreator creator);
    
    // Create strategy instance
    std::unique_ptr<BaseStrategy> create_strategy(const std::string& type, 
                                                 const std::string& name,
                                                 const StrategyConfig& config);
    
    // Get available strategy types
    std::vector<std::string> get_available_strategies() const;

private:
    StrategyFactory() = default;
    std::map<std::string, StrategyCreator> creators_;
    mutable std::mutex mutex_;
};

// Strategy registration macro
#define REGISTER_STRATEGY(type, class_name) \
    static bool class_name##_registered = []() { \
        StrategyFactory::get_instance().register_strategy(type, \
            [](const std::string& name, const StrategyConfig& config) { \
                return std::make_unique<class_name>(name, config); \
            }); \
        return true; \
    }()

} // namespace mev
