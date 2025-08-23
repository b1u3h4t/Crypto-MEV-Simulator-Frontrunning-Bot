#include "config.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>

namespace mev {

ConfigManager::ConfigManager() {
    // Initialize with default values
    load_default_config();
}

void ConfigManager::load_config(const std::string& file_path) {
    try {
        std::ifstream file(file_path);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open config file: " + file_path);
        }
        
        nlohmann::json j;
        file >> j;
        
        from_json(j);
        validate();
        
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to load config from " + file_path + ": " + e.what());
    }
}

void ConfigManager::load_default_config() {
    // Load from default config file
    load_config("config/default_config.json");
}

void ConfigManager::override_config(const SimulationConfig& cli_config) {
    // Override simulation mode
    if (cli_config.mode != SimulationMode::REALTIME) {
        config.simulation.mode = cli_config.mode;
    }
    
    // Override enabled strategies if specified
    if (!cli_config.enabled_strategies.empty()) {
        // Disable all strategies first
        for (auto& [name, strategy] : config.strategies.strategies) {
            strategy.enabled = false;
        }
        
        // Enable only specified strategies
        for (const auto& strategy_name : cli_config.enabled_strategies) {
            auto& strategy = config.strategies.strategies[strategy_name];
            strategy.enabled = true;
        }
    }
    
    // Override fork settings
    if (!cli_config.fork_url.empty()) {
        config.blockchain.fork.enabled = true;
        config.blockchain.fork.url = cli_config.fork_url;
        config.blockchain.fork.block_number = cli_config.fork_block;
    }
    
    // Override simulation parameters
    if (cli_config.start_block > 0) {
        // This would be used in historical mode
    }
    
    if (cli_config.block_count > 0) {
        // This would be used in historical mode
    }
    
    if (cli_config.duration_seconds > 0) {
        config.simulation.synthetic_data.duration_seconds = cli_config.duration_seconds;
    }
    
    if (cli_config.tx_rate > 0) {
        config.simulation.synthetic_data.transaction_rate = cli_config.tx_rate;
    }
}

const StrategyConfig& ConfigManager::get_strategy_config(const std::string& strategy_name) const {
    return config.strategies.get_strategy(strategy_name);
}

bool ConfigManager::is_strategy_enabled(const std::string& strategy_name) const {
    return config.strategies.is_strategy_enabled(strategy_name);
}

std::vector<std::string> ConfigManager::get_enabled_strategies() const {
    std::vector<std::string> enabled;
    for (const auto& [name, strategy] : config.strategies.strategies) {
        if (strategy.enabled) {
            enabled.push_back(name);
        }
    }
    return enabled;
}

void ConfigManager::validate() const {
    // Validate simulation settings
    if (config.simulation.synthetic_data.enabled && 
        config.simulation.synthetic_data.transaction_rate == 0) {
        throw std::runtime_error("Transaction rate must be > 0 when synthetic data is enabled");
    }
    
    // Validate performance settings
    if (config.performance.thread_pool_size == 0) {
        throw std::runtime_error("Thread pool size must be > 0");
    }
    
    if (config.performance.queue_size == 0) {
        throw std::runtime_error("Queue size must be > 0");
    }
    
    // Validate blockchain settings
    if (config.blockchain.ethereum.rpc_url.empty()) {
        throw std::runtime_error("Ethereum RPC URL is required");
    }
    
    // Validate strategy settings
    for (const auto& [name, strategy] : config.strategies.strategies) {
        if (strategy.enabled) {
            if (strategy.min_profit_eth < 0) {
                throw std::runtime_error("Strategy " + name + ": min_profit_eth must be >= 0");
            }
            
            if (strategy.max_slippage_percent < 0 || strategy.max_slippage_percent > 100) {
                throw std::runtime_error("Strategy " + name + ": max_slippage_percent must be between 0 and 100");
            }
            
            if (strategy.gas_limit == 0) {
                throw std::runtime_error("Strategy " + name + ": gas_limit must be > 0");
            }
        }
    }
    
    // Validate monitoring settings
    if (config.monitoring.metrics.enabled && config.monitoring.metrics.port == 0) {
        throw std::runtime_error("Metrics port must be > 0 when metrics are enabled");
    }
}

nlohmann::json ConfigManager::to_json() const {
    nlohmann::json j;
    j["simulation"] = simulation_to_json();
    j["performance"] = performance_to_json();
    j["strategies"] = strategies_to_json();
    j["blockchain"] = blockchain_to_json();
    j["trading"] = trading_to_json();
    j["monitoring"] = monitoring_to_json();
    j["data"] = data_to_json();
    j["security"] = security_to_json();
    return j;
}

void ConfigManager::from_json(const nlohmann::json& j) {
    if (j.contains("simulation")) parse_simulation_config(j["simulation"]);
    if (j.contains("performance")) parse_performance_config(j["performance"]);
    if (j.contains("strategies")) parse_strategies_config(j["strategies"]);
    if (j.contains("blockchain")) parse_blockchain_config(j["blockchain"]);
    if (j.contains("trading")) parse_trading_config(j["trading"]);
    if (j.contains("monitoring")) parse_monitoring_config(j["monitoring"]);
    if (j.contains("data")) parse_data_config(j["data"]);
    if (j.contains("security")) parse_security_config(j["security"]);
}

// JSON parsing methods
void ConfigManager::parse_simulation_config(const nlohmann::json& j) {
    if (j.contains("mode")) {
        std::string mode = j["mode"];
        if (mode == "realtime") config.simulation.mode = SimulationMode::REALTIME;
        else if (mode == "historical") config.simulation.mode = SimulationMode::HISTORICAL;
        else if (mode == "synthetic") config.simulation.mode = SimulationMode::SYNTHETIC;
    }
    
    if (j.contains("mempool_emulation")) config.simulation.mempool_emulation = j["mempool_emulation"];
    if (j.contains("block_simulation")) config.simulation.block_simulation = j["block_simulation"];
    if (j.contains("historical_replay")) config.simulation.historical_replay = j["historical_replay"];
    
    if (j.contains("synthetic_data")) {
        auto& sd = j["synthetic_data"];
        if (sd.contains("enabled")) config.simulation.synthetic_data.enabled = sd["enabled"];
        if (sd.contains("transaction_rate")) config.simulation.synthetic_data.transaction_rate = sd["transaction_rate"];
        if (sd.contains("duration_seconds")) config.simulation.synthetic_data.duration_seconds = sd["duration_seconds"];
    }
}

void ConfigManager::parse_performance_config(const nlohmann::json& j) {
    if (j.contains("thread_pool_size")) config.performance.thread_pool_size = j["thread_pool_size"];
    if (j.contains("queue_size")) config.performance.queue_size = j["queue_size"];
    if (j.contains("latency_target_us")) config.performance.latency_target_us = j["latency_target_us"];
    if (j.contains("max_concurrent_requests")) config.performance.max_concurrent_requests = j["max_concurrent_requests"];
    if (j.contains("enable_simd")) config.performance.enable_simd = j["enable_simd"];
    if (j.contains("memory_mapping")) config.performance.memory_mapping = j["memory_mapping"];
}

void ConfigManager::parse_strategies_config(const nlohmann::json& j) {
    for (const auto& [name, strategy_json] : j.items()) {
        StrategyConfig strategy;
        
        if (strategy_json.contains("enabled")) strategy.enabled = strategy_json["enabled"];
        if (strategy_json.contains("min_profit_eth")) strategy.min_profit_eth = strategy_json["min_profit_eth"];
        if (strategy_json.contains("max_slippage_percent")) strategy.max_slippage_percent = strategy_json["max_slippage_percent"];
        if (strategy_json.contains("target_dexes")) strategy.target_dexes = strategy_json["target_dexes"];
        if (strategy_json.contains("gas_limit")) strategy.gas_limit = strategy_json["gas_limit"];
        if (strategy_json.contains("max_gas_price_gwei")) strategy.max_gas_price_gwei = strategy_json["max_gas_price_gwei"];
        if (strategy_json.contains("bundle_timeout_ms")) strategy.bundle_timeout_ms = strategy_json["bundle_timeout_ms"];
        if (strategy_json.contains("frontrun_gas_multiplier")) strategy.frontrun_gas_multiplier = strategy_json["frontrun_gas_multiplier"];
        if (strategy_json.contains("backrun_gas_multiplier")) strategy.backrun_gas_multiplier = strategy_json["backrun_gas_multiplier"];
        if (strategy_json.contains("priority_fee_gwei")) strategy.priority_fee_gwei = strategy_json["priority_fee_gwei"];
        if (strategy_json.contains("min_transaction_value_eth")) strategy.min_transaction_value_eth = strategy_json["min_transaction_value_eth"];
        if (strategy_json.contains("target_protocols")) strategy.target_protocols = strategy_json["target_protocols"];
        
        config.strategies.strategies[name] = strategy;
    }
}

void ConfigManager::parse_blockchain_config(const nlohmann::json& j) {
    if (j.contains("ethereum")) {
        auto& eth = j["ethereum"];
        if (eth.contains("rpc_url")) config.blockchain.ethereum.rpc_url = eth["rpc_url"];
        if (eth.contains("chain_id")) config.blockchain.ethereum.chain_id = eth["chain_id"];
        if (eth.contains("block_time_seconds")) config.blockchain.ethereum.block_time_seconds = eth["block_time_seconds"];
    }
    
    if (j.contains("flashbots")) {
        auto& fb = j["flashbots"];
        if (fb.contains("relay_url")) config.blockchain.flashbots.relay_url = fb["relay_url"];
        if (fb.contains("bundle_timeout_ms")) config.blockchain.flashbots.bundle_timeout_ms = fb["bundle_timeout_ms"];
        if (fb.contains("max_bundle_size")) config.blockchain.flashbots.max_bundle_size = fb["max_bundle_size"];
    }
    
    if (j.contains("fork")) {
        auto& fork = j["fork"];
        if (fork.contains("enabled")) config.blockchain.fork.enabled = fork["enabled"];
        if (fork.contains("url")) config.blockchain.fork.url = fork["url"];
        if (fork.contains("block_number")) config.blockchain.fork.block_number = fork["block_number"];
    }
}

void ConfigManager::parse_trading_config(const nlohmann::json& j) {
    if (j.contains("gas_optimization")) {
        auto& go = j["gas_optimization"];
        if (go.contains("enabled")) config.trading.gas_optimization.enabled = go["enabled"];
        if (go.contains("base_fee_multiplier")) config.trading.gas_optimization.base_fee_multiplier = go["base_fee_multiplier"];
        if (go.contains("priority_fee_strategy")) config.trading.gas_optimization.priority_fee_strategy = go["priority_fee_strategy"];
        if (go.contains("max_gas_price_gwei")) config.trading.gas_optimization.max_gas_price_gwei = go["max_gas_price_gwei"];
    }
    
    if (j.contains("slippage")) {
        auto& slip = j["slippage"];
        if (slip.contains("default_percent")) config.trading.slippage.default_percent = slip["default_percent"];
        if (slip.contains("max_percent")) config.trading.slippage.max_percent = slip["max_percent"];
        if (slip.contains("dynamic_adjustment")) config.trading.slippage.dynamic_adjustment = slip["dynamic_adjustment"];
    }
    
    if (j.contains("bundle")) {
        auto& bundle = j["bundle"];
        if (bundle.contains("max_transactions")) config.trading.bundle.max_transactions = bundle["max_transactions"];
        if (bundle.contains("timeout_ms")) config.trading.bundle.timeout_ms = bundle["timeout_ms"];
        if (bundle.contains("retry_attempts")) config.trading.bundle.retry_attempts = bundle["retry_attempts"];
    }
}

void ConfigManager::parse_monitoring_config(const nlohmann::json& j) {
    if (j.contains("metrics")) {
        auto& metrics = j["metrics"];
        if (metrics.contains("enabled")) config.monitoring.metrics.enabled = metrics["enabled"];
        if (metrics.contains("port")) config.monitoring.metrics.port = metrics["port"];
        if (metrics.contains("export_interval_seconds")) config.monitoring.metrics.export_interval_seconds = metrics["export_interval_seconds"];
    }
    
    if (j.contains("logging")) {
        auto& logging = j["logging"];
        if (logging.contains("level")) config.monitoring.logging.level = logging["level"];
        if (logging.contains("file")) config.monitoring.logging.file = logging["file"];
        if (logging.contains("max_file_size_mb")) config.monitoring.logging.max_file_size_mb = logging["max_file_size_mb"];
        if (logging.contains("max_files")) config.monitoring.logging.max_files = logging["max_files"];
    }
    
    if (j.contains("visualization")) {
        auto& viz = j["visualization"];
        if (viz.contains("enabled")) config.monitoring.visualization.enabled = viz["enabled"];
        if (viz.contains("update_interval_ms")) config.monitoring.visualization.update_interval_ms = viz["update_interval_ms"];
        if (viz.contains("export_formats")) config.monitoring.visualization.export_formats = viz["export_formats"];
    }
}

void ConfigManager::parse_data_config(const nlohmann::json& j) {
    if (j.contains("storage")) {
        auto& storage = j["storage"];
        if (storage.contains("type")) config.data.storage.type = storage["type"];
        if (storage.contains("directory")) config.data.storage.directory = storage["directory"];
        if (storage.contains("compression")) config.data.storage.compression = storage["compression"];
    }
    
    if (j.contains("cache")) {
        auto& cache = j["cache"];
        if (cache.contains("enabled")) config.data.cache.enabled = cache["enabled"];
        if (cache.contains("ttl_seconds")) config.data.cache.ttl_seconds = cache["ttl_seconds"];
        if (cache.contains("max_size_mb")) config.data.cache.max_size_mb = cache["max_size_mb"];
    }
    
    if (j.contains("export")) {
        auto& export_settings = j["export"];
        if (export_settings.contains("formats")) config.data.export_settings.formats = export_settings["formats"];
        if (export_settings.contains("include_metrics")) config.data.export_settings.include_metrics = export_settings["include_metrics"];
        if (export_settings.contains("timestamp_format")) config.data.export_settings.timestamp_format = export_settings["timestamp_format"];
    }
}

void ConfigManager::parse_security_config(const nlohmann::json& j) {
    if (j.contains("rate_limiting")) {
        auto& rl = j["rate_limiting"];
        if (rl.contains("enabled")) config.security.rate_limiting.enabled = rl["enabled"];
        if (rl.contains("requests_per_second")) config.security.rate_limiting.requests_per_second = rl["requests_per_second"];
        if (rl.contains("burst_size")) config.security.rate_limiting.burst_size = rl["burst_size"];
    }
    
    if (j.contains("validation")) {
        auto& val = j["validation"];
        if (val.contains("enable_ssl_verification")) config.security.validation.enable_ssl_verification = val["enable_ssl_verification"];
        if (val.contains("max_request_size_mb")) config.security.validation.max_request_size_mb = val["max_request_size_mb"];
        if (val.contains("timeout_ms")) config.security.validation.timeout_ms = val["timeout_ms"];
    }
}

// JSON serialization methods
nlohmann::json ConfigManager::simulation_to_json() const {
    nlohmann::json j;
    switch (config.simulation.mode) {
        case SimulationMode::REALTIME: j["mode"] = "realtime"; break;
        case SimulationMode::HISTORICAL: j["mode"] = "historical"; break;
        case SimulationMode::SYNTHETIC: j["mode"] = "synthetic"; break;
    }
    j["mempool_emulation"] = config.simulation.mempool_emulation;
    j["block_simulation"] = config.simulation.block_simulation;
    j["historical_replay"] = config.simulation.historical_replay;
    
    nlohmann::json synthetic_data;
    synthetic_data["enabled"] = config.simulation.synthetic_data.enabled;
    synthetic_data["transaction_rate"] = config.simulation.synthetic_data.transaction_rate;
    synthetic_data["duration_seconds"] = config.simulation.synthetic_data.duration_seconds;
    j["synthetic_data"] = synthetic_data;
    
    return j;
}

nlohmann::json ConfigManager::performance_to_json() const {
    nlohmann::json j;
    j["thread_pool_size"] = config.performance.thread_pool_size;
    j["queue_size"] = config.performance.queue_size;
    j["latency_target_us"] = config.performance.latency_target_us;
    j["max_concurrent_requests"] = config.performance.max_concurrent_requests;
    j["enable_simd"] = config.performance.enable_simd;
    j["memory_mapping"] = config.performance.memory_mapping;
    return j;
}

nlohmann::json ConfigManager::strategies_to_json() const {
    nlohmann::json j;
    for (const auto& [name, strategy] : config.strategies.strategies) {
        nlohmann::json strategy_json;
        strategy_json["enabled"] = strategy.enabled;
        strategy_json["min_profit_eth"] = strategy.min_profit_eth;
        strategy_json["max_slippage_percent"] = strategy.max_slippage_percent;
        strategy_json["target_dexes"] = strategy.target_dexes;
        strategy_json["gas_limit"] = strategy.gas_limit;
        strategy_json["max_gas_price_gwei"] = strategy.max_gas_price_gwei;
        strategy_json["bundle_timeout_ms"] = strategy.bundle_timeout_ms;
        strategy_json["frontrun_gas_multiplier"] = strategy.frontrun_gas_multiplier;
        strategy_json["backrun_gas_multiplier"] = strategy.backrun_gas_multiplier;
        strategy_json["priority_fee_gwei"] = strategy.priority_fee_gwei;
        strategy_json["min_transaction_value_eth"] = strategy.min_transaction_value_eth;
        strategy_json["target_protocols"] = strategy.target_protocols;
        j[name] = strategy_json;
    }
    return j;
}

nlohmann::json ConfigManager::blockchain_to_json() const {
    nlohmann::json j;
    
    nlohmann::json ethereum;
    ethereum["rpc_url"] = config.blockchain.ethereum.rpc_url;
    ethereum["chain_id"] = config.blockchain.ethereum.chain_id;
    ethereum["block_time_seconds"] = config.blockchain.ethereum.block_time_seconds;
    j["ethereum"] = ethereum;
    
    nlohmann::json flashbots;
    flashbots["relay_url"] = config.blockchain.flashbots.relay_url;
    flashbots["bundle_timeout_ms"] = config.blockchain.flashbots.bundle_timeout_ms;
    flashbots["max_bundle_size"] = config.blockchain.flashbots.max_bundle_size;
    j["flashbots"] = flashbots;
    
    nlohmann::json fork;
    fork["enabled"] = config.blockchain.fork.enabled;
    fork["url"] = config.blockchain.fork.url;
    fork["block_number"] = config.blockchain.fork.block_number;
    j["fork"] = fork;
    
    return j;
}

nlohmann::json ConfigManager::trading_to_json() const {
    nlohmann::json j;
    
    nlohmann::json gas_optimization;
    gas_optimization["enabled"] = config.trading.gas_optimization.enabled;
    gas_optimization["base_fee_multiplier"] = config.trading.gas_optimization.base_fee_multiplier;
    gas_optimization["priority_fee_strategy"] = config.trading.gas_optimization.priority_fee_strategy;
    gas_optimization["max_gas_price_gwei"] = config.trading.gas_optimization.max_gas_price_gwei;
    j["gas_optimization"] = gas_optimization;
    
    nlohmann::json slippage;
    slippage["default_percent"] = config.trading.slippage.default_percent;
    slippage["max_percent"] = config.trading.slippage.max_percent;
    slippage["dynamic_adjustment"] = config.trading.slippage.dynamic_adjustment;
    j["slippage"] = slippage;
    
    nlohmann::json bundle;
    bundle["max_transactions"] = config.trading.bundle.max_transactions;
    bundle["timeout_ms"] = config.trading.bundle.timeout_ms;
    bundle["retry_attempts"] = config.trading.bundle.retry_attempts;
    j["bundle"] = bundle;
    
    return j;
}

nlohmann::json ConfigManager::monitoring_to_json() const {
    nlohmann::json j;
    
    nlohmann::json metrics;
    metrics["enabled"] = config.monitoring.metrics.enabled;
    metrics["port"] = config.monitoring.metrics.port;
    metrics["export_interval_seconds"] = config.monitoring.metrics.export_interval_seconds;
    j["metrics"] = metrics;
    
    nlohmann::json logging;
    logging["level"] = config.monitoring.logging.level;
    logging["file"] = config.monitoring.logging.file;
    logging["max_file_size_mb"] = config.monitoring.logging.max_file_size_mb;
    logging["max_files"] = config.monitoring.logging.max_files;
    j["logging"] = logging;
    
    nlohmann::json visualization;
    visualization["enabled"] = config.monitoring.visualization.enabled;
    visualization["update_interval_ms"] = config.monitoring.visualization.update_interval_ms;
    visualization["export_formats"] = config.monitoring.visualization.export_formats;
    j["visualization"] = visualization;
    
    return j;
}

nlohmann::json ConfigManager::data_to_json() const {
    nlohmann::json j;
    
    nlohmann::json storage;
    storage["type"] = config.data.storage.type;
    storage["directory"] = config.data.storage.directory;
    storage["compression"] = config.data.storage.compression;
    j["storage"] = storage;
    
    nlohmann::json cache;
    cache["enabled"] = config.data.cache.enabled;
    cache["ttl_seconds"] = config.data.cache.ttl_seconds;
    cache["max_size_mb"] = config.data.cache.max_size_mb;
    j["cache"] = cache;
    
    nlohmann::json export_settings;
    export_settings["formats"] = config.data.export_settings.formats;
    export_settings["include_metrics"] = config.data.export_settings.include_metrics;
    export_settings["timestamp_format"] = config.data.export_settings.timestamp_format;
    j["export"] = export_settings;
    
    return j;
}

nlohmann::json ConfigManager::security_to_json() const {
    nlohmann::json j;
    
    nlohmann::json rate_limiting;
    rate_limiting["enabled"] = config.security.rate_limiting.enabled;
    rate_limiting["requests_per_second"] = config.security.rate_limiting.requests_per_second;
    rate_limiting["burst_size"] = config.security.rate_limiting.burst_size;
    j["rate_limiting"] = rate_limiting;
    
    nlohmann::json validation;
    validation["enable_ssl_verification"] = config.security.validation.enable_ssl_verification;
    validation["max_request_size_mb"] = config.security.validation.max_request_size_mb;
    validation["timeout_ms"] = config.security.validation.timeout_ms;
    j["validation"] = validation;
    
    return j;
}

} // namespace mev
