#include <iostream>
#include <memory>
#include <signal.h>
#include <chrono>
#include <thread>

#include "config.hpp"
#include "simulator/Simulator.hpp"
#include "utils/Logger.hpp"
#include "utils/Metrics.hpp"

using namespace mev;

// Global variables for graceful shutdown
std::unique_ptr<Simulator> g_simulator;
std::atomic<bool> g_shutdown_requested{false};

// Signal handler for graceful shutdown
void signal_handler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down gracefully..." << std::endl;
    g_shutdown_requested.store(true);
    
    if (g_simulator) {
        g_simulator->stop();
    }
}

// Print usage information
void print_usage(const char* program_name) {
    std::cout << "MEV Frontrunning Bot - Crypto MEV Simulator\n\n"
              << "Usage: " << program_name << " [OPTIONS]\n\n"
              << "Options:\n"
              << "  --mode <mode>              Simulation mode (realtime, historical, synthetic)\n"
              << "  --config <file>            Configuration file path\n"
              << "  --strategies <list>        Comma-separated list of strategies\n"
              << "  --block <number>           Starting block number (historical mode)\n"
              << "  --blocks <count>           Number of blocks to simulate\n"
              << "  --duration <seconds>       Simulation duration (synthetic mode)\n"
              << "  --tx-rate <rate>           Transaction rate per second (synthetic mode)\n"
              << "  --visualize                Enable real-time visualization\n"
              << "  --profile                  Enable performance profiling\n"
              << "  --export-csv               Export results to CSV\n"
              << "  --export-json              Export results to JSON\n"
              << "  --fork-url <url>           Fork URL for local testing\n"
              << "  --fork-block <number>      Fork block number\n"
              << "  --help                     Show this help message\n\n"
              << "Examples:\n"
              << "  " << program_name << " --mode realtime --strategies arbitrage,sandwich\n"
              << "  " << program_name << " --mode historical --block 15000000 --blocks 100\n"
              << "  " << program_name << " --mode synthetic --duration 3600 --tx-rate 1000\n"
              << "  " << program_name << " --config custom_config.json --visualize\n";
}

// Parse command line arguments
SimulationConfig parse_arguments(int argc, char* argv[]) {
    SimulationConfig config;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            exit(0);
        } else if (arg == "--mode" && i + 1 < argc) {
            std::string mode = argv[++i];
            if (mode == "realtime") {
                config.mode = SimulationMode::REALTIME;
            } else if (mode == "historical") {
                config.mode = SimulationMode::HISTORICAL;
            } else if (mode == "synthetic") {
                config.mode = SimulationMode::SYNTHETIC;
            } else {
                throw std::runtime_error("Invalid simulation mode: " + mode);
            }
        } else if (arg == "--config" && i + 1 < argc) {
            config.config_file = argv[++i];
        } else if (arg == "--strategies" && i + 1 < argc) {
            std::string strategies = argv[++i];
            // Parse comma-separated strategy list
            size_t pos = 0;
            while ((pos = strategies.find(',')) != std::string::npos) {
                config.enabled_strategies.push_back(strategies.substr(0, pos));
                strategies.erase(0, pos + 1);
            }
            if (!strategies.empty()) {
                config.enabled_strategies.push_back(strategies);
            }
        } else if (arg == "--block" && i + 1 < argc) {
            config.start_block = std::stoull(argv[++i]);
        } else if (arg == "--blocks" && i + 1 < argc) {
            config.block_count = std::stoull(argv[++i]);
        } else if (arg == "--duration" && i + 1 < argc) {
            config.duration_seconds = std::stoull(argv[++i]);
        } else if (arg == "--tx-rate" && i + 1 < argc) {
            config.tx_rate = std::stoull(argv[++i]);
        } else if (arg == "--visualize") {
            config.enable_visualization = true;
        } else if (arg == "--profile") {
            config.enable_profiling = true;
        } else if (arg == "--export-csv") {
            config.export_formats.push_back("csv");
        } else if (arg == "--export-json") {
            config.export_formats.push_back("json");
        } else if (arg == "--fork-url" && i + 1 < argc) {
            config.fork_url = argv[++i];
        } else if (arg == "--fork-block" && i + 1 < argc) {
            config.fork_block = std::stoull(argv[++i]);
        } else {
            throw std::runtime_error("Unknown argument: " + arg);
        }
    }
    
    return config;
}

int main(int argc, char* argv[]) {
    try {
        // Set up signal handlers for graceful shutdown
        signal(SIGINT, signal_handler);
        signal(SIGTERM, signal_handler);
        
        // Parse command line arguments
        SimulationConfig cli_config = parse_arguments(argc, argv);
        
        // Initialize configuration
        ConfigManager config_manager;
        if (!cli_config.config_file.empty()) {
            config_manager.load_config(cli_config.config_file);
        } else {
            config_manager.load_default_config();
        }
        
        // Override config with CLI arguments
        config_manager.override_config(cli_config);
        
        // Initialize logging
        Logger::initialize(config_manager.get_logging_config());
        Logger::info("Starting MEV Frontrunning Bot...");
        
        // Initialize metrics
        if (config_manager.get_monitoring_config().metrics.enabled) {
            Metrics::initialize(config_manager.get_monitoring_config().metrics);
            Logger::info("Metrics initialized on port " + 
                        std::to_string(config_manager.get_monitoring_config().metrics.port));
        }
        
        // Create and start simulator
        g_simulator = std::make_unique<Simulator>(config_manager);
        
        Logger::info("Initializing simulator...");
        g_simulator->initialize();
        
        Logger::info("Starting simulation...");
        g_simulator->start();
        
        // Main event loop
        while (!g_shutdown_requested.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            // Check if simulator is still running
            if (!g_simulator->is_running()) {
                Logger::warn("Simulator stopped unexpectedly");
                break;
            }
        }
        
        // Graceful shutdown
        Logger::info("Shutting down...");
        g_simulator->stop();
        g_simulator->wait_for_completion();
        
        // Export results if requested
        if (!cli_config.export_formats.empty()) {
            Logger::info("Exporting results...");
            g_simulator->export_results(cli_config.export_formats);
        }
        
        Logger::info("MEV Frontrunning Bot stopped successfully");
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        Logger::error("Fatal error: " + std::string(e.what()));
        return 1;
    } catch (...) {
        std::cerr << "Unknown fatal error occurred" << std::endl;
        Logger::error("Unknown fatal error occurred");
        return 1;
    }
}
