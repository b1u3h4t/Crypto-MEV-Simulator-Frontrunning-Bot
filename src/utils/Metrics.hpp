#pragma once

#include <string>
#include <map>
#include <vector>
#include <atomic>
#include <mutex>
#include <chrono>
#include <memory>
#include <thread>
#include <functional>

namespace mev {

// Metric types
enum class MetricType {
    COUNTER,
    GAUGE,
    HISTOGRAM,
    SUMMARY
};

// Metric value structure
struct MetricValue {
    double value = 0.0;
    std::chrono::system_clock::time_point timestamp;
    std::map<std::string, std::string> labels;
};

// Histogram bucket
struct HistogramBucket {
    double upper_bound;
    uint64_t count;
};

// Summary quantile
struct SummaryQuantile {
    double quantile;
    double value;
};

// Base metric class
class Metric {
public:
    Metric(const std::string& name, const std::string& help, MetricType type);
    virtual ~Metric() = default;
    
    const std::string& get_name() const { return name_; }
    const std::string& get_help() const { return help_; }
    MetricType get_type() const { return type_; }
    
    virtual void reset() = 0;
    virtual std::string to_prometheus() const = 0;
    virtual nlohmann::json to_json() const = 0;

protected:
    std::string name_;
    std::string help_;
    MetricType type_;
    mutable std::mutex mutex_;
};

// Counter metric
class Counter : public Metric {
public:
    Counter(const std::string& name, const std::string& help = "");
    
    void increment(double value = 1.0);
    void reset() override;
    double get_value() const;
    
    std::string to_prometheus() const override;
    nlohmann::json to_json() const override;

private:
    std::atomic<double> value_{0.0};
    std::chrono::system_clock::time_point last_update_;
};

// Gauge metric
class Gauge : public Metric {
public:
    Gauge(const std::string& name, const std::string& help = "");
    
    void set(double value);
    void increment(double value = 1.0);
    void decrement(double value = 1.0);
    void reset() override;
    double get_value() const;
    
    std::string to_prometheus() const override;
    nlohmann::json to_json() const override;

private:
    std::atomic<double> value_{0.0};
    std::chrono::system_clock::time_point last_update_;
};

// Histogram metric
class Histogram : public Metric {
public:
    Histogram(const std::string& name, const std::string& help = "", 
              const std::vector<double>& buckets = {0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1, 2.5, 5, 10});
    
    void observe(double value);
    void reset() override;
    
    double get_sum() const;
    uint64_t get_count() const;
    double get_bucket_count(double upper_bound) const;
    
    std::string to_prometheus() const override;
    nlohmann::json to_json() const override;

private:
    std::atomic<double> sum_{0.0};
    std::atomic<uint64_t> count_{0};
    std::vector<double> buckets_;
    std::vector<std::atomic<uint64_t>> bucket_counts_;
    std::atomic<uint64_t> infinity_bucket_{0};
};

// Summary metric
class Summary : public Metric {
public:
    Summary(const std::string& name, const std::string& help = "",
            const std::vector<double>& quantiles = {0.5, 0.9, 0.95, 0.99});
    
    void observe(double value);
    void reset() override;
    
    double get_sum() const;
    uint64_t get_count() const;
    double get_quantile(double q) const;
    
    std::string to_prometheus() const override;
    nlohmann::json to_json() const override;

private:
    std::atomic<double> sum_{0.0};
    std::atomic<uint64_t> count_{0};
    std::vector<double> quantiles_;
    std::vector<double> values_;
    mutable std::mutex values_mutex_;
};

// Timer utility class
class Timer {
public:
    Timer(Histogram& histogram);
    ~Timer();
    
    void stop();

private:
    Histogram& histogram_;
    std::chrono::high_resolution_clock::time_point start_time_;
    bool stopped_ = false;
};

// Metrics registry
class MetricsRegistry {
public:
    static MetricsRegistry& get_instance();
    
    // Metric creation
    Counter& create_counter(const std::string& name, const std::string& help = "");
    Gauge& create_gauge(const std::string& name, const std::string& help = "");
    Histogram& create_histogram(const std::string& name, const std::string& help = "",
                               const std::vector<double>& buckets = {});
    Summary& create_summary(const std::string& name, const std::string& help = "",
                           const std::vector<double>& quantiles = {});
    
    // Metric access
    Counter& get_counter(const std::string& name);
    Gauge& get_gauge(const std::string& name);
    Histogram& get_histogram(const std::string& name);
    Summary& get_summary(const std::string& name);
    
    // Registry operations
    void reset_all();
    std::string to_prometheus() const;
    nlohmann::json to_json() const;
    
    // Get all metrics
    std::vector<std::string> get_metric_names() const;

private:
    MetricsRegistry() = default;
    ~MetricsRegistry() = default;
    MetricsRegistry(const MetricsRegistry&) = delete;
    MetricsRegistry& operator=(const MetricsRegistry&) = delete;
    
    std::map<std::string, std::unique_ptr<Metric>> metrics_;
    mutable std::mutex mutex_;
};

// Metrics server configuration
struct MetricsConfig {
    bool enabled = true;
    uint16_t port = 8080;
    std::string endpoint = "/metrics";
    uint32_t export_interval_seconds = 60;
    bool prometheus_format = true;
    bool json_format = false;
};

// Metrics server
class MetricsServer {
public:
    MetricsServer(const MetricsConfig& config);
    ~MetricsServer();
    
    void start();
    void stop();
    bool is_running() const;
    
    // Export metrics
    void export_metrics(const std::string& format = "prometheus");
    void export_to_file(const std::string& filename, const std::string& format = "prometheus");

private:
    MetricsConfig config_;
    std::unique_ptr<std::thread> server_thread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> stop_requested_{false};
    
    void server_loop();
    void handle_request(const std::string& request, std::string& response);
    std::string generate_response(const std::string& format);
};

// Convenience functions
void Metrics::initialize(const MetricsConfig& config);
void Metrics::shutdown();

// Global metrics
extern MetricsRegistry& metrics;

// Convenience macros for metrics
#define METRICS_COUNTER(name, help) mev::metrics.create_counter(name, help)
#define METRICS_GAUGE(name, help) mev::metrics.create_gauge(name, help)
#define METRICS_HISTOGRAM(name, help, buckets) mev::metrics.create_histogram(name, help, buckets)
#define METRICS_SUMMARY(name, help, quantiles) mev::metrics.create_summary(name, help, quantiles)

#define METRICS_TIMER(name) mev::Timer timer(mev::metrics.get_histogram(name))

} // namespace mev
