#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include <learned_index/data/key.hpp>
#include <learned_index/data/sorted_dataset.hpp>
#include <learned_index/index/index.hpp>

namespace learned_index {

// A static piecewise-linear learned index. Each rank-sized segment has its own
// normalized regression model; exact lookup is bounded to that segment and
// corrected with exponential bracketing plus binary search.
//
// The dataset is borrowed and must outlive this index.
class PiecewiseLearnedIndex {
private:
    struct Segment {
        std::size_t first_position;
        std::size_t last_position;
        Key first_key;
        Key last_key;
        long double intercept;
        long double slope;
    };

    const SortedDataset& dataset_;
    std::size_t target_segment_size_;
    std::vector<Segment> segments_;

    void train();
    [[nodiscard]] const Segment* segment_for(Key key) const noexcept;

public:
    explicit PiecewiseLearnedIndex(
        const SortedDataset& dataset,
        std::size_t target_segment_size = 1024
    );

    [[nodiscard]] std::size_t predict_position(Key key) const noexcept;
    [[nodiscard]] std::optional<std::size_t> find(Key key) const noexcept;
    [[nodiscard]] bool contains(Key key) const noexcept;
};

static_assert(Index<PiecewiseLearnedIndex>);

} // namespace learned_index
