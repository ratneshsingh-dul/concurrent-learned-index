#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include <learned_index/data/key.hpp>
#include <learned_index/data/sorted_dataset.hpp>
#include <learned_index/index/index.hpp>

namespace learned_index {

// A two-stage Recursive Model Index (RMI) for a static sorted dataset. The
// root model predicts a rank and routes to a leaf model; the leaf predicts a
// candidate rank. Exact exponential bracketing and binary correction make
// lookup correct even when the learned routing selects a neighboring leaf.
// The dataset is borrowed and must outlive this index.
class RmiLearnedIndex {
private:
    struct LinearModel {
        std::size_t first_position;
        std::size_t last_position;
        Key first_key;
        Key last_key;
        long double intercept;
        long double slope;
    };

    const SortedDataset& dataset_;
    std::size_t target_leaf_size_;
    LinearModel root_{0, 0, 0, 0, 0.0L, 0.0L};
    std::vector<LinearModel> leaves_;

    void train();
    [[nodiscard]] static LinearModel fit_model(
        const SortedDataset& dataset,
        std::size_t first_position,
        std::size_t last_position
    ) noexcept;
    [[nodiscard]] static std::size_t predict(
        const LinearModel& model,
        Key key
    ) noexcept;

public:
    explicit RmiLearnedIndex(
        const SortedDataset& dataset,
        std::size_t target_leaf_size = 1024
    );

    [[nodiscard]] std::size_t predict_position(Key key) const noexcept;
    [[nodiscard]] std::optional<std::size_t> find(Key key) const noexcept;
    [[nodiscard]] bool contains(Key key) const noexcept;
};

static_assert(Index<RmiLearnedIndex>);

} // namespace learned_index
