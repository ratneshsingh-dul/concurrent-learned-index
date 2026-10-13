#include <learned_index/index/piecewise_learned_index.hpp>

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace learned_index {
namespace {

std::size_t lower_bound_index(
    const SortedDataset& dataset,
    std::size_t first,
    std::size_t last,
    Key key
) noexcept {
    while (first < last) {
        const std::size_t middle = first + (last - first) / 2;
        if (dataset[middle] < key) {
            first = middle + 1;
        } else {
            last = middle;
        }
    }
    return first;
}

} // namespace

PiecewiseLearnedIndex::PiecewiseLearnedIndex(
    const SortedDataset& dataset,
    std::size_t target_segment_size
)
    : dataset_(dataset), target_segment_size_(target_segment_size) {
    if (target_segment_size_ == 0) {
        throw std::invalid_argument("target_segment_size must be greater than zero");
    }
    train();
}

void PiecewiseLearnedIndex::train() {
    segments_.clear();
    if (dataset_.empty()) {
        return;
    }

    const std::size_t segment_count = dataset_.size() / target_segment_size_ +
        (dataset_.size() % target_segment_size_ != 0);
    segments_.reserve(segment_count);

    for (std::size_t first = 0; first < dataset_.size();) {
        const std::size_t remaining = dataset_.size() - first;
        const std::size_t width = std::min(target_segment_size_, remaining);
        const std::size_t last = first + width - 1;
        const Key first_key = dataset_[first];
        const Key key_span = dataset_[last] - first_key;
        const std::size_t rank_span = last - first;

        long double intercept = 0.0L;
        long double slope = 0.0L;
        if (rank_span != 0 && key_span != 0) {
            long double mean_x = 0.0L;
            long double mean_y = 0.0L;
            long double sum_xx = 0.0L;
            long double sum_xy = 0.0L;
            std::size_t count = 0;

            for (std::size_t position = first; position <= last; ++position) {
                const long double x = static_cast<long double>(
                    dataset_[position] - first_key
                ) / static_cast<long double>(key_span);
                const long double y = static_cast<long double>(position - first) /
                    static_cast<long double>(rank_span);
                ++count;
                const long double delta_x = x - mean_x;
                const long double delta_y = y - mean_y;
                mean_x += delta_x / static_cast<long double>(count);
                mean_y += delta_y / static_cast<long double>(count);
                sum_xx += delta_x * (x - mean_x);
                sum_xy += delta_x * (y - mean_y);
            }

            if (sum_xx > 0.0L) {
                slope = sum_xy / sum_xx;
                intercept = mean_y - slope * mean_x;
            }
        }

        segments_.push_back(Segment{
            first,
            last,
            first_key,
            dataset_[last],
            intercept,
            slope
        });
        first += width;
    }
}

const PiecewiseLearnedIndex::Segment* PiecewiseLearnedIndex::segment_for(
    Key key
) const noexcept {
    const auto position = std::lower_bound(
        segments_.begin(), segments_.end(), key,
        [](const Segment& segment, Key value) {
            return segment.last_key < value;
        }
    );
    return position == segments_.end() ? nullptr : &*position;
}

std::size_t PiecewiseLearnedIndex::predict_position(Key key) const noexcept {
    if (dataset_.empty() || key <= dataset_[0]) {
        return 0;
    }
    const std::size_t last = dataset_.size() - 1;
    if (key >= dataset_[last]) {
        return last;
    }

    const Segment* segment = segment_for(key);
    if (segment == nullptr) {
        return last;
    }
    if (key <= segment->first_key) {
        return segment->first_position;
    }

    const std::size_t rank_span = segment->last_position - segment->first_position;
    if (rank_span == 0) {
        return segment->first_position;
    }
    const Key key_span = segment->last_key - segment->first_key;
    const long double normalized_key = static_cast<long double>(
        key - segment->first_key
    ) / static_cast<long double>(key_span);
    const long double raw_local_position =
        (segment->intercept + segment->slope * normalized_key) *
        static_cast<long double>(rank_span);

    if (raw_local_position <= 0.0L) {
        return segment->first_position;
    }
    if (raw_local_position >= static_cast<long double>(rank_span)) {
        return segment->last_position;
    }
    return segment->first_position + static_cast<std::size_t>(
        std::floor(raw_local_position + 0.5L)
    );
}

std::optional<std::size_t> PiecewiseLearnedIndex::find(Key key) const noexcept {
    if (dataset_.empty() || key < dataset_[0] || key > dataset_[dataset_.size() - 1]) {
        return std::nullopt;
    }

    const Segment* segment = segment_for(key);
    if (segment == nullptr || key < segment->first_key) {
        return std::nullopt;
    }

    const std::size_t first_position = segment->first_position;
    const std::size_t end_position = segment->last_position + 1;
    const std::size_t predicted = predict_position(key);
    if (dataset_[predicted] == key) {
        return predicted;
    }

    std::size_t position = predicted;
    std::size_t step = 1;
    if (dataset_[predicted] < key) {
        std::size_t first = predicted + 1;
        std::size_t probe = first;
        while (probe < end_position && dataset_[probe] < key) {
            first = probe + 1;
            if (first == end_position) {
                return std::nullopt;
            }
            const std::size_t remaining = end_position - 1 - probe;
            probe += std::min(step, remaining);
            step = step > dataset_.size() / 2 ? dataset_.size() : step * 2;
        }
        position = lower_bound_index(dataset_, first, probe + 1, key);
    } else {
        std::size_t first = predicted;
        std::size_t last = predicted + 1;
        while (first > first_position && dataset_[first - 1] > key) {
            last = first;
            const std::size_t distance = first - first_position;
            first -= std::min(step, distance);
            step = step > dataset_.size() / 2 ? dataset_.size() : step * 2;
        }
        if (first > first_position) {
            --first;
        }
        position = lower_bound_index(dataset_, first, last, key);
    }

    if (position < end_position && dataset_[position] == key) {
        return position;
    }
    return std::nullopt;
}

bool PiecewiseLearnedIndex::contains(Key key) const noexcept {
    return find(key).has_value();
}

} // namespace learned_index
