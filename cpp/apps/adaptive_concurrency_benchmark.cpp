#include <algorithm>
#include <atomic>
#include <barrier>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <learned_index/data/key.hpp>
#include <learned_index/data/sorted_dataset.hpp>
#include <learned_index/index/adaptive_learned_index.hpp>
#include <benchmark_dataset.hpp>

namespace {

using Clock = std::chrono::steady_clock;

benchmark_data::DatasetShape parse_shape(const char* text) {
    const std::string shape(text);
    if (shape == "even") return benchmark_data::DatasetShape::evenly_spaced;
    if (shape == "clustered") return benchmark_data::DatasetShape::clustered;
    if (shape == "quadratic") return benchmark_data::DatasetShape::quadratic_gaps;
    if (shape == "random") return benchmark_data::DatasetShape::random_64;
    throw std::invalid_argument("shape must be even, clustered, quadratic, or random");
}

std::uint64_t parse_count(const char* text, const char* name) {
    const std::string value(text);
    std::size_t consumed = 0;
    const auto parsed = std::stoull(value, &consumed);
    if (consumed != value.size() || parsed == 0) {
        throw std::invalid_argument(std::string(name) + " must be a positive integer");
    }
    return parsed;
}

double percentile_us(std::vector<double> samples, double percentile) {
    if (samples.empty()) {
        return 0.0;
    }
    std::sort(samples.begin(), samples.end());
    const auto index = static_cast<std::size_t>(
        percentile * static_cast<double>(samples.size() - 1)
    );
    return samples[index];
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc < 4 || argc > 7) {
            std::cerr << "Usage: adaptive_concurrency_benchmark "
                         "<reader_threads> <dataset_size> <writer_inserts> "
                         "[global|rmi|auto] [rmi_leaf_size] "
                         "[even|clustered|quadratic|random]\n";
            return 2;
        }
        const auto reader_count = static_cast<std::size_t>(
            parse_count(argv[1], "reader_threads")
        );
        const auto dataset_size = static_cast<std::size_t>(
            parse_count(argv[2], "dataset_size")
        );
        const auto writer_inserts = static_cast<std::size_t>(
            parse_count(argv[3], "writer_inserts")
        );
        auto model_kind = learned_index::AdaptiveLearnedIndex::ModelKind::global;
        std::size_t rmi_leaf_size = 1024;
        benchmark_data::DatasetShape shape =
            benchmark_data::DatasetShape::evenly_spaced;
        if (argc >= 5) {
            const std::string model_name(argv[4]);
            if (model_name == "rmi") {
                model_kind = learned_index::AdaptiveLearnedIndex::ModelKind::rmi;
                if (argc == 6) {
                    rmi_leaf_size = static_cast<std::size_t>(
                        parse_count(argv[5], "rmi_leaf_size")
                    );
                }
            } else if (model_name == "auto") {
                model_kind = learned_index::AdaptiveLearnedIndex::ModelKind::automatic;
                if (argc >= 6) {
                    rmi_leaf_size = static_cast<std::size_t>(
                        parse_count(argv[5], "rmi_leaf_size")
                    );
                }
            } else if (model_name == "global") {
                if (argc >= 6) {
                    rmi_leaf_size = static_cast<std::size_t>(
                        parse_count(argv[5], "rmi_leaf_size")
                    );
                }
            } else {
                throw std::invalid_argument("model must be global, rmi, or auto");
            }
        }
        if (argc == 7) {
            shape = parse_shape(argv[6]);
        }

        auto dataset = benchmark_data::make_dataset(dataset_size, shape);
        const auto max_key = dataset[dataset.size() - 1];
        if (writer_inserts > std::numeric_limits<learned_index::Key>::max() - max_key) {
            throw std::invalid_argument("requested inserted key range overflows uint64_t");
        }
        learned_index::AdaptiveLearnedIndex index(
            dataset, 256,
            model_kind, rmi_leaf_size
        );

        std::barrier start(static_cast<std::ptrdiff_t>(reader_count + 1));
        std::atomic<bool> writer_done{false};
        std::atomic<std::uint64_t> total_reads{0};
        std::vector<std::thread> readers;
        readers.reserve(reader_count);
        for (std::size_t thread_index = 0; thread_index < reader_count; ++thread_index) {
            readers.emplace_back([&] {
                std::uint64_t reads = 0;
                // Warm the lookup path before releasing the writer and timed
                // read loop together. This reduces startup scheduling noise.
                for (std::size_t warmup = 0; warmup < 1024; ++warmup) {
                    if (!index.contains(dataset[warmup % dataset_size])) {
                        std::terminate();
                    }
                }
                start.arrive_and_wait();
                while (!writer_done.load(std::memory_order_acquire)) {
                    const auto position = static_cast<std::size_t>(
                        reads % dataset_size
                    );
                    if (!index.contains(dataset[position])) {
                        std::cerr << "Reader observed a missing base key\n";
                        std::terminate();
                    }
                    ++reads;
                }
                total_reads.fetch_add(reads, std::memory_order_relaxed);
            });
        }

        std::vector<double> insert_latency_us;
        insert_latency_us.reserve(writer_inserts);
        std::vector<double> rebuild_latency_us;
        rebuild_latency_us.reserve(writer_inserts / 256);
        start.arrive_and_wait();
        const auto run_start = Clock::now();
        for (std::size_t i = 0; i < writer_inserts; ++i) {
            const auto key = max_key + static_cast<learned_index::Key>(i) + 1;
            const auto insert_start = Clock::now();
            if (!index.insert(key)) {
                std::cerr << "Writer unexpectedly rejected a unique key\n";
                std::terminate();
            }
            insert_latency_us.push_back(
                std::chrono::duration<double, std::micro>(Clock::now() - insert_start).count()
            );
            if ((i + 1) % 256 == 0) {
                rebuild_latency_us.push_back(insert_latency_us.back());
            }
        }
        writer_done.store(true, std::memory_order_release);
        for (auto& reader : readers) {
            reader.join();
        }
        const auto elapsed_seconds = std::chrono::duration<double>(
            Clock::now() - run_start
        ).count();

        bool exact = index.size() == dataset_size + writer_inserts;
        for (std::size_t i = 0; exact && i < writer_inserts; ++i) {
            const auto key = max_key + static_cast<learned_index::Key>(i) + 1;
            exact = index.contains(key);
        }
        const auto reads = total_reads.load(std::memory_order_relaxed);
        const auto selected_model = index.selected_model_kind();
        const char* model_name = model_kind ==
            learned_index::AdaptiveLearnedIndex::ModelKind::rmi ? "rmi" :
            model_kind == learned_index::AdaptiveLearnedIndex::ModelKind::automatic
                ? "auto" : "global";
        const char* selected_model_name = selected_model ==
            learned_index::AdaptiveLearnedIndex::ModelKind::rmi ? "rmi" : "global";
        std::cout << "model_policy,selected_model,shape,reader_threads,dataset_size,"
                     "writer_inserts,rmi_leaf_size,"
                     "read_ops,elapsed_seconds,"
                     "read_ops_per_second,writer_p50_us,writer_p95_us,rebuild_p50_us,"
                     "rebuild_p95_us,rebuild_max_us,rebuild_count,exact\n";
        std::cout << model_name << ',' << selected_model_name << ','
                  << benchmark_data::shape_name(shape) << ',' << reader_count
                  << ',' << dataset_size << ',' << writer_inserts << ',' << rmi_leaf_size << ','
                  << reads << ',' << elapsed_seconds << ','
                  << static_cast<double>(reads) / elapsed_seconds << ','
                  << percentile_us(insert_latency_us, 0.50) << ','
                  << percentile_us(insert_latency_us, 0.95) << ','
                  << percentile_us(rebuild_latency_us, 0.50) << ','
                  << percentile_us(rebuild_latency_us, 0.95) << ','
                  << (rebuild_latency_us.empty() ? 0.0 : *std::max_element(
                      rebuild_latency_us.begin(), rebuild_latency_us.end()
                  )) << ','
                  << index.rebuild_count() << ',' << (exact ? "true" : "false") << '\n';
        return exact ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 2;
    }
}
