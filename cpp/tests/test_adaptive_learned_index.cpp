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

TEST(AdaptiveLearnedIndexTest, RejectsZeroRmiLeafSize) {
    EXPECT_THROW(
        AdaptiveLearnedIndex(
            SortedDataset({}), 8, AdaptiveLearnedIndex::ModelKind::rmi, 0
        ),
        std::invalid_argument
    );
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

TEST(AdaptiveLearnedIndexTest, RebuildsWithRmiModelAndKeepsExactMembership) {
    AdaptiveLearnedIndex index(
        SortedDataset({10, 20, 30, 40}), 3,
        AdaptiveLearnedIndex::ModelKind::rmi, 2
    );
    for (const Key key : {25, 5, 50, 35, 60, 15, 45}) {
        EXPECT_TRUE(index.insert(key));
        EXPECT_TRUE(index.contains(key));
    }
    EXPECT_EQ(index.size(), 11);
    EXPECT_GT(index.rebuild_count(), 1);
    EXPECT_EQ(index.pending_size(), 1);
    for (const Key key : {5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 60}) {
        EXPECT_TRUE(index.contains(key));
    }
    for (const Key key : {0, 12, 55, 70}) {
        EXPECT_FALSE(index.contains(key));
    }
    EXPECT_FALSE(index.insert(25));
}

TEST(AdaptiveLearnedIndexTest, AutomaticPolicyKeepsGlobalForLinearKeys) {
    std::vector<Key> keys;
    for (Key i = 0; i < 512; ++i) {
        keys.push_back(i * 10);
    }
    AdaptiveLearnedIndex index(
        SortedDataset(std::move(keys)), 8,
        AdaptiveLearnedIndex::ModelKind::automatic, 32
    );
    EXPECT_EQ(index.selected_model_kind(), AdaptiveLearnedIndex::ModelKind::global);
    for (Key i = 512; i < 520; ++i) {
        EXPECT_TRUE(index.insert(i * 10));
    }
    EXPECT_EQ(index.selected_model_kind(), AdaptiveLearnedIndex::ModelKind::global);
    EXPECT_EQ(index.rebuild_count(), 1);
}

TEST(AdaptiveLearnedIndexTest, AutomaticPolicySelectsRmiForQuadraticKeys) {
    std::vector<Key> keys;
    for (Key i = 0; i < 1024; ++i) {
        keys.push_back(i * i);
    }
    AdaptiveLearnedIndex index(
        SortedDataset(std::move(keys)), 32,
        AdaptiveLearnedIndex::ModelKind::automatic, 64
    );
    EXPECT_EQ(index.selected_model_kind(), AdaptiveLearnedIndex::ModelKind::rmi);
    for (Key i = 0; i < 1024; ++i) {
        EXPECT_TRUE(index.contains(i * i));
    }
    EXPECT_FALSE(index.contains(2048));
    for (Key i = 1024; i < 1056; ++i) {
        EXPECT_TRUE(index.insert(i * i));
    }
    EXPECT_EQ(index.selected_model_kind(), AdaptiveLearnedIndex::ModelKind::rmi);
    EXPECT_EQ(index.rebuild_count(), 1);
}

TEST(AdaptiveLearnedIndexTest, AutomaticPolicyCanChangeModelAfterRebuild) {
    std::vector<Key> keys;
    for (Key i = 0; i < 10000; ++i) {
        keys.push_back(i * 2);
    }
    AdaptiveLearnedIndex index(
        SortedDataset(std::move(keys)), 256,
        AdaptiveLearnedIndex::ModelKind::automatic, 128
    );
    EXPECT_EQ(index.selected_model_kind(), AdaptiveLearnedIndex::ModelKind::global);
    for (Key i = 0; i < 256; ++i) {
        EXPECT_TRUE(index.insert(20000 + i));
    }
    EXPECT_EQ(index.rebuild_count(), 1);
    EXPECT_EQ(index.selected_model_kind(), AdaptiveLearnedIndex::ModelKind::rmi);
    EXPECT_TRUE(index.contains(20000));
    EXPECT_TRUE(index.contains(20255));
}

TEST(AdaptiveLearnedIndexTest, SwitchesModelDuringConcurrentReadsAndRebuild) {
    std::vector<Key> initial_keys;
    initial_keys.reserve(100000);
    for (Key i = 0; i < 100000; ++i) {
        initial_keys.push_back(i * 2);
    }
    AdaptiveLearnedIndex index(
        SortedDataset(std::move(initial_keys)), 256,
        AdaptiveLearnedIndex::ModelKind::automatic, 128
    );
    ASSERT_EQ(index.selected_model_kind(), AdaptiveLearnedIndex::ModelKind::global);

    std::atomic<bool> start{false};
    std::atomic<bool> writer_done{false};
    std::atomic<std::size_t> published_inserts{0};
    std::atomic<std::size_t> reader_failures{0};
    auto reader = [&] {
        while (!start.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        do {
            if (!index.contains(0) || !index.contains(199998)) {
                reader_failures.fetch_add(1, std::memory_order_relaxed);
            }
            const auto visible = published_inserts.load(std::memory_order_acquire);
            if (visible > 0 && !index.contains(200000 + visible - 1)) {
                reader_failures.fetch_add(1, std::memory_order_relaxed);
            }
        } while (!writer_done.load(std::memory_order_acquire));
    };

    std::thread first_reader(reader);
    std::thread second_reader(reader);
    std::thread writer([&] {
        start.store(true, std::memory_order_release);
        for (Key i = 0; i < 256; ++i) {
            if (!index.insert(200000 + i)) {
                reader_failures.fetch_add(1, std::memory_order_relaxed);
            }
            published_inserts.store(static_cast<std::size_t>(i + 1),
                                    std::memory_order_release);
        }
        writer_done.store(true, std::memory_order_release);
    });

    writer.join();
    first_reader.join();
    second_reader.join();

    EXPECT_EQ(reader_failures.load(), 0);
    EXPECT_EQ(index.rebuild_count(), 1);
    EXPECT_EQ(index.selected_model_kind(), AdaptiveLearnedIndex::ModelKind::rmi);
    for (Key i = 0; i < 256; ++i) {
        EXPECT_TRUE(index.contains(200000 + i));
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
