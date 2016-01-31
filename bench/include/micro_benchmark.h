#ifndef MICRO_BENCHMARK_H_
#define MICRO_BENCHMARK_H_

#include <chrono>

#ifdef MICA_SERVER
#include "mica_store.h"
#else
#include "log_store.h"
#endif

#ifdef MICA_SERVER
using namespace ::mica;
#else
using namespace ::succinct;
#endif

using namespace ::std::chrono;

class MicroBenchmark {
 public:

  typedef unsigned long long int TimeStamp;

  static const uint64_t kWarmupCount = 1000;
  static const uint64_t kMeasureCount = 100000;
  static const uint64_t kCooldownCount = 1000;

  static const uint64_t kWarmupTime = 15000000;
  static const uint64_t kMeasureTime = 60000000;
  static const uint64_t kCooldownTime = 5000000;

  static const uint64_t kThreadQueryCount = 100000;

  MicroBenchmark(std::string& data_path, int mode = 0);  // Mode: 0 = Load from scratch, 1 = Load from dump

  // Latency benchmarks
  void BenchmarkGetLatency();
  void BenchmarkSearchLatency();
  void BenchmarkAppendLatency();

  // Throughput benchmarks
  void BenchmarkThroughput(double get_f, double search_f, double append_f,
                           uint32_t num_clients = 1);

 private:
  static TimeStamp GetTimestamp() {
    struct timeval now;
    gettimeofday(&now, NULL);

    return now.tv_usec + (TimeStamp) now.tv_sec * 1000000;
  }

  std::string data_path_;
  int64_t load_end_offset_;
  int64_t load_keys_;

#ifdef MICA_SERVER
  MicaStore *shard_;
#else
  LogStore *shard_;
#endif
};

#endif
