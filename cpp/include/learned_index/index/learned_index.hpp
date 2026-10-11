#pragma once

#include <cstddef>
#include <optional>

#include <learned_index/data/key.hpp>
#include <learned_index/data/sorted_dataset.hpp>

namespace learned_index {

// A static learned index using one least-squares linear model from normalized
// uint64 keys to normalized positions. The model is only a hint: find() uses
// exponential bracketing and exact binary correction, so prediction errors
// cannot change lookup correctness.
//
// The dataset is borrowed and must outlive this index. SortedDataset is
// immutable through its public API, so the trained mapping cannot go stale.
class LearnedIndex {
private:
    const SortedDataset& dataset_;
    long double intercept_ = 0.0L;
    long double slope_ = 0.0L;

    void train() noexcept;

public:
    explicit LearnedIndex(const SortedDataset& dataset) noexcept;

    [[nodiscard]] std::size_t predict_position(Key key) const noexcept;
    [[nodiscard]] std::optional<std::size_t> find(Key key) const noexcept;
    [[nodiscard]] bool contains(Key key) const noexcept;
};

} // namespace learned_index
