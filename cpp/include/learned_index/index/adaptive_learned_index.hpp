#pragma once

#include <cstddef>
#include <memory>
#include <set>
#include <shared_mutex>

#include <learned_index/data/key.hpp>
#include <learned_index/data/sorted_dataset.hpp>
#include <learned_index/index/index.hpp>
#include <learned_index/index/learned_index.hpp>

namespace learned_index {

// An insert-only adaptive learned index for unique keys. A sorted immutable
// base is searched through LearnedIndex while recent inserts live in an
// ordered delta. Reaching rebuild_threshold merges the delta and retrains.
// All public methods are thread-safe. Rebuild computation uses a stable base
// snapshot and captured delta outside the lock; publication takes the writer
// lock briefly, so readers continue against the old snapshot during training.
class AdaptiveLearnedIndex {
private:
    struct Snapshot {
        SortedDataset dataset;
        LearnedIndex model;

        explicit Snapshot(SortedDataset data);
    };

    mutable std::shared_mutex mutex_;
    std::shared_ptr<const Snapshot> snapshot_;
    std::set<Key> delta_;
    std::size_t size_;
    std::size_t rebuild_threshold_;
    std::size_t rebuild_count_ = 0;
    bool rebuild_in_progress_ = false;

public:
    explicit AdaptiveLearnedIndex(
        SortedDataset initial,
        std::size_t rebuild_threshold = 1024
    );

    [[nodiscard]] bool contains(Key key) const;
    [[nodiscard]] bool insert(Key key);
    [[nodiscard]] std::size_t size() const;
    [[nodiscard]] std::size_t pending_size() const;
    [[nodiscard]] std::size_t rebuild_count() const;
};

static_assert(Index<AdaptiveLearnedIndex>);

} // namespace learned_index
