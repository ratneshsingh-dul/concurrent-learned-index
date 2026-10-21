#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <random>
#include <set>
#include <stdexcept>
#include <thread>
#include <vector>

#include <learned_index/data/sorted_dataset.hpp>
#include <learned_index/index/adaptive_learned_index.hpp>

namespace learned_index {

TEST(AdaptiveLearnedIndexTest, RejectsZeroRebuildThreshold) {
    EXPECT_THROW(AdaptiveLearnedIndex(SortedDataset({}), 0), std::invalid_argument);
}

TEST(AdaptiveLearnedIndexTest, InsertsInDeltaAndRebuildsAtThreshold) {
    AdaptiveLearnedIndex index(SortedDataset({10, 20, 30}), 3);
    EXPECT_TRUE(index.insert(25));
    EXPECT_TRUE(index.contains(25));
    EXPECT_EQ(index.pending_size(), 1);
    EXPECT_EQ(index.rebuild_count(), 0);
    EXPECT_FALSE(index.insert(25));
    EXPECT_FALSE(index.insert(20));

    EXPECT_TRUE(index.insert(5));
    EXPECT_TRUE(index.insert(40));
    EXPECT_EQ(index.rebuild_count(), 1);
    EXPECT_EQ(index.pending_size(), 0);
    EXPECT_EQ(index.size(), 6);
    for (const Key key : {5, 10, 20, 25, 30, 40}) {
        EXPECT_TRUE(index.contains(key));
    }
}

TEST(AdaptiveLearnedIndexTest, RandomizedUpdatesMatchOrderedSetAcrossRebuilds) {
    AdaptiveLearnedIndex index(SortedDataset({}), 11);
    std::set<Key> reference;
    std::vector<Key> keys;
    for (std::uint64_t i = 0; i < 2000; ++i) {
        keys.push_back(i * 19);
    }
    std::mt19937_64 random(991827);
    std::shuffle(keys.begin(), keys.end(), random);
    for (const Key key : keys) {
        EXPECT_EQ(index.insert(key), reference.insert(key).second);
        EXPECT_FALSE(index.insert(key));
    }
    EXPECT_EQ(index.size(), reference.size());
    EXPECT_GT(index.rebuild_count(), 100);
    for (std::uint64_t i = 0; i < 4000; ++i) {
        const Key key = i * 7;
        EXPECT_EQ(index.contains(key), reference.contains(key));
    }
}

TEST(AdaptiveLearnedIndexTest, SupportsConcurrentReadersAndWriter) {
    std::vector<Key> initial_keys;
    for (Key key = 0; key < 256; ++key) {
        initial_keys.push_back(key * 2);
    }
    AdaptiveLearnedIndex index(SortedDataset(std::move(initial_keys)), 13);
    std::atomic<bool> start{false};
    std::atomic<std::size_t> reader_failures{0};
    std::atomic<std::size_t> writer_failures{0};
    std::vector<std::thread> readers;
    for (std::size_t thread = 0; thread < 4; ++thread) {
        readers.emplace_back([&] {
            while (!start.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            for (std::size_t pass = 0; pass < 100; ++pass) {
                for (Key key = 0; key < 256; ++key) {
                    if (!index.contains(key * 2)) {
                        reader_failures.fetch_add(1, std::memory_order_relaxed);
                    }
                }
            }
        });
    }
    std::vector<std::thread> writers;
    for (const Key first : {Key{512}, Key{2000}}) {
        writers.emplace_back([&, first] {
            start.store(true, std::memory_order_release);
            for (Key key = first; key < first + 1000; ++key) {
                if (!index.insert(key * 2)) {
                    writer_failures.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }
    for (auto& writer : writers) {
        writer.join();
    }
    for (auto& reader : readers) {
        reader.join();
    }

    EXPECT_EQ(reader_failures.load(), 0);
    EXPECT_EQ(writer_failures.load(), 0);
    EXPECT_EQ(index.size(), 2256);
    for (Key key = 0; key < 256; ++key) {
        EXPECT_TRUE(index.contains(key * 2));
    }
    for (Key key = 512; key < 1512; ++key) {
        EXPECT_TRUE(index.contains(key * 2));
    }
    for (Key key = 2000; key < 3000; ++key) {
        EXPECT_TRUE(index.contains(key * 2));
    }
}

} // namespace learned_index
