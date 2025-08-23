#pragma once

#include "BaseStrategy.hpp"
#include <map>
#include <vector>
#include <string>

namespace mev {

// DEX types
enum class DexType {
    UNISWAP_V2,
    UNISWAP_V3,
    SUSHISWAP,
    BALANCER,
    CURVE,
    BANCOR
};

// Token pair information
struct TokenPair {
    std::string token0;
    std::string token1;
    std::string pair_address;
    DexType dex_type;
    double reserve0;
    double reserve1;
    double fee_percent;
    double price;
    
    // Calculate price based on reserves
    double calculate_price() const {
        if (reserve1 == 0) return 0.0;
        return reserve0 / reserve1;
    }
    
    // Calculate output amount for given input
    double calculate_output_amount(double input_amount, bool is_token0_to_token1) const {
        if (is_token0_to_token1) {
            if (reserve0 == 0 || reserve1 == 0) return 0.0;
            double fee_multiplier = 1.0 - (fee_percent / 100.0);
            double input_with_fee = input_amount * fee_multiplier;
            return (input_with_fee * reserve1) / (reserve0 + input_with_fee);
        } else {
            if (reserve1 == 0 || reserve0 == 0) return 0.0;
            double fee_multiplier = 1.0 - (fee_percent / 100.0);
            double input_with_fee = input_amount * fee_multiplier;
            return (input_with_fee * reserve0) / (reserve1 + input_with_fee);
        }
    }
};

// Arbitrage path
struct ArbitragePath {
    std::vector<std::string> tokens;
    std::vector<TokenPair> pairs;
    double expected_profit_eth;
    double required_input_eth;
    double gas_estimate;
    
    // Calculate optimal input amount for maximum profit
    double calculate_optimal_input() const;
    
    // Validate path
    bool is_valid() const {
        return tokens.size() >= 3 && pairs.size() >= 2 && expected_profit_eth > 0;
    }
};

// Price feed interface
class PriceFeed {
public:
    virtual ~PriceFeed() = default;
    virtual double get_token_price(const std::string& token_address) = 0;
    virtual std::map<std::string, double> get_token_prices(const std::vector<std::string>& token_addresses) = 0;
    virtual void update_prices() = 0;
};

// Arbitrage strategy implementation
class ArbitrageStrategy : public BaseStrategy {
public:
    explicit ArbitrageStrategy(const std::string& name, const StrategyConfig& config);
    ~ArbitrageStrategy() override = default;
    
    // Strategy interface implementation
    bool detect_opportunity(const StrategyContext& context, Opportunity& opportunity) override;
    StrategyResult execute_opportunity(const Opportunity& opportunity, Bundle& bundle) override;
    
    // Arbitrage-specific methods
    void add_dex(DexType dex_type, const std::string& factory_address);
    void add_token_pair(const TokenPair& pair);
    void set_price_feed(std::unique_ptr<PriceFeed> price_feed);
    
    // Configuration
    void set_min_profit_threshold(double min_profit_eth);
    void set_max_path_length(uint32_t max_length);
    void set_max_gas_price(uint64_t max_gas_price_gwei);
    
    // Analysis methods
    std::vector<ArbitragePath> find_arbitrage_paths(const std::vector<std::string>& target_tokens);
    double calculate_path_profit(const ArbitragePath& path, double input_amount);
    bool validate_path_execution(const ArbitragePath& path);

protected:
    // Override base class methods
    void initialize() override;
    void shutdown() override;
    void reset() override;
    
    // Validation
    bool validate_opportunity(const Opportunity& opportunity) const override;
    bool validate_bundle(const Bundle& bundle) const override;

private:
    // DEX and token data
    std::map<DexType, std::string> dex_factories_;
    std::map<std::string, TokenPair> token_pairs_;
    std::unique_ptr<PriceFeed> price_feed_;
    
    // Configuration
    double min_profit_threshold_eth_;
    uint32_t max_path_length_;
    uint64_t max_gas_price_gwei_;
    
    // Cached data
    std::map<std::string, double> cached_prices_;
    std::chrono::system_clock::time_point last_price_update_;
    
    // Internal methods
    void initialize_dex_connections();
    void update_token_pairs();
    void update_price_cache();
    
    // Path finding algorithms
    std::vector<ArbitragePath> find_triangular_arbitrage(const std::vector<std::string>& base_tokens);
    std::vector<ArbitragePath> find_cross_dex_arbitrage(const std::vector<std::string>& tokens);
    std::vector<ArbitragePath> find_complex_arbitrage(const std::vector<std::string>& tokens);
    
    // Path analysis
    double calculate_optimal_input_amount(const ArbitragePath& path);
    double estimate_execution_profit(const ArbitragePath& path, double input_amount);
    double calculate_slippage_impact(const ArbitragePath& path, double input_amount);
    
    // Transaction building
    std::vector<Transaction> build_arbitrage_transactions(const ArbitragePath& path, double input_amount);
    Transaction build_swap_transaction(const TokenPair& pair, double input_amount, bool is_token0_to_token1);
    
    // Gas estimation
    uint64_t estimate_path_gas(const ArbitragePath& path);
    uint64_t estimate_swap_gas(const TokenPair& pair);
    
    // Risk management
    bool is_path_safe(const ArbitragePath& path);
    double calculate_risk_score(const ArbitragePath& path);
    bool check_liquidity_sufficiency(const ArbitragePath& path, double input_amount);
    
    // Utility methods
    std::string generate_opportunity_id(const ArbitragePath& path);
    std::map<std::string, std::string> extract_path_metadata(const ArbitragePath& path);
    
    // Logging
    void log_path_discovery(const ArbitragePath& path);
    void log_path_execution(const ArbitragePath& path, StrategyResult result);
    
    // Performance optimization
    void precompute_common_paths();
    void cache_frequently_used_pairs();
    
    // Error handling
    void handle_dex_error(DexType dex_type, const std::string& error);
    void handle_price_feed_error(const std::string& error);
    
    // Metrics
    void initialize_arbitrage_metrics();
    void update_arbitrage_metrics(const ArbitragePath& path, StrategyResult result);
    
    // Arbitrage-specific metrics
    std::unique_ptr<Counter> triangular_arbitrage_counter_;
    std::unique_ptr<Counter> cross_dex_arbitrage_counter_;
    std::unique_ptr<Counter> complex_arbitrage_counter_;
    std::unique_ptr<Histogram> path_length_histogram_;
    std::unique_ptr<Histogram> profit_margin_histogram_;
    std::unique_ptr<Gauge> active_dex_count_gauge_;
    std::unique_ptr<Gauge> cached_pairs_count_gauge_;
};

// Simple price feed implementation
class SimplePriceFeed : public PriceFeed {
public:
    SimplePriceFeed();
    ~SimplePriceFeed() override = default;
    
    double get_token_price(const std::string& token_address) override;
    std::map<std::string, double> get_token_prices(const std::vector<std::string>& token_addresses) override;
    void update_prices() override;
    
    // Manual price updates
    void set_token_price(const std::string& token_address, double price);
    void set_token_prices(const std::map<std::string, double>& prices);

private:
    std::map<std::string, double> prices_;
    mutable std::mutex prices_mutex_;
};

} // namespace mev
