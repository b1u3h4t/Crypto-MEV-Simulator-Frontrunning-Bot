#pragma once

#include <memory>
#include <atomic>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>

#include "config.hpp"
#include "utils/Logger.hpp"
#include "utils/Metrics.hpp"

namespace mev {

// Forward declarations
class MempoolEmulator;
class BlockBuilder;
class StrategyEngine;
class TransactionBuilder;
class BlockchainInterface;
class PerformanceVisualizer;

// Simulation state
enum class SimulationState {
    INITIALIZING,
    RUNNING,
    PAUSED,
    STOPPING,
    STOPPED,
    ERROR
};

// Simulation statistics
struct SimulationStats {
    uint64_t blocks_processed = 0;
    uint64_t transactions_processed = 0;
    uint64_t strategies_executed = 0;
    uint64_t profitable_opportunities = 0;
    double total_profit_eth = 0.0;
    double total_gas_used = 0.0;
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point last_update;
    
    // Latency statistics
    double avg_mempool_latency_us = 0.0;
    double avg_strategy_detection_latency_us = 0.0;
    double avg_transaction_build_latency_us = 0.0;
    double avg_bundle_submission_latency_us = 0.0;
    
    // Throughput statistics
    double tx_per_second = 0.0;
    double strategies_per_second = 0.0;
    double opportunities_per_second = 0.0;
};

// Main simulator class
class Simulator {
public:
    explicit Simulator(const ConfigManager& config_manager);
    ~Simulator();
    
    // Lifecycle methods
    void initialize();
    void start();
    void stop();
    void pause();
    void resume();
    void wait_for_completion();
    
    // State queries
    SimulationState get_state() const;
    bool is_running() const;
    bool is_paused() const;
    
    // Statistics and monitoring
    SimulationStats get_stats() const;
    void update_stats();
    void reset_stats();
    
    // Configuration
    void update_config(const ConfigManager& config_manager);
    const ConfigManager& get_config() const;
    
    // Results and export
    void export_results(const std::vector<std::string>& formats);
    void save_simulation_state(const std::string& filename);
    void load_simulation_state(const std::string& filename);
    
    // Component access
    MempoolEmulator& get_mempool_emulator();
    BlockBuilder& get_block_builder();
    StrategyEngine& get_strategy_engine();
    TransactionBuilder& get_transaction_builder();
    BlockchainInterface& get_blockchain_interface();
    PerformanceVisualizer& get_visualizer();

private:
    // Configuration
    ConfigManager config_manager_;
    
    // Core components
    std::unique_ptr<MempoolEmulator> mempool_emulator_;
    std::unique_ptr<BlockBuilder> block_builder_;
    std::unique_ptr<StrategyEngine> strategy_engine_;
    std::unique_ptr<TransactionBuilder> transaction_builder_;
    std::unique_ptr<BlockchainInterface> blockchain_interface_;
    std::unique_ptr<PerformanceVisualizer> visualizer_;
    
    // Threading and synchronization
    std::atomic<SimulationState> state_{SimulationState::STOPPED};
    std::unique_ptr<std::thread> main_thread_;
    std::unique_ptr<std::thread> stats_thread_;
    std::unique_ptr<std::thread> visualization_thread_;
    
    std::mutex state_mutex_;
    std::condition_variable state_cv_;
    std::atomic<bool> stop_requested_{false};
    std::atomic<bool> pause_requested_{false};
    
    // Statistics and monitoring
    mutable std::mutex stats_mutex_;
    SimulationStats stats_;
    std::chrono::system_clock::time_point last_stats_update_;
    
    // Performance metrics
    std::unique_ptr<Counter> blocks_processed_counter_;
    std::unique_ptr<Counter> transactions_processed_counter_;
    std::unique_ptr<Counter> strategies_executed_counter_;
    std::unique_ptr<Counter> profitable_opportunities_counter_;
    std::unique_ptr<Gauge> total_profit_gauge_;
    std::unique_ptr<Gauge> total_gas_used_gauge_;
    std::unique_ptr<Histogram> mempool_latency_histogram_;
    std::unique_ptr<Histogram> strategy_detection_latency_histogram_;
    std::unique_ptr<Histogram> transaction_build_latency_histogram_;
    std::unique_ptr<Histogram> bundle_submission_latency_histogram_;
    
    // Internal methods
    void main_loop();
    void stats_loop();
    void visualization_loop();
    
    void initialize_components();
    void shutdown_components();
    
    void process_block(uint64_t block_number);
    void process_transaction(const std::string& tx_hash);
    void execute_strategies();
    
    void update_performance_metrics();
    void log_performance_metrics();
    
    // Error handling
    void handle_error(const std::string& error_message);
    void recover_from_error();
    
    // Configuration validation
    void validate_configuration() const;
    
    // Performance optimization
    void optimize_performance();
    void adjust_thread_pool_size();
    
    // Memory management
    void cleanup_memory();
    void check_memory_usage();
};

} // namespace mev
