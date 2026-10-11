#include <learned_index/index/learned_index.hpp>

#include <algorithm>
#include <cmath>

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

LearnedIndex::LearnedIndex(const SortedDataset& dataset) noexcept
    : dataset_(dataset) {
    train();
}

void LearnedIndex::train() noexcept {
    if (dataset_.size() < 2) {
        return;
    }

    const Key first_key = dataset_[0];
    const Key key_span = dataset_[dataset_.size() - 1] - first_key;
    if (key_span == 0) {
        return;
    }

    // Welford-style online covariance avoids summing large raw uint64 keys.
    // Both model inputs are normalized to [0, 1].
    long double mean_x = 0.0L;
    long double mean_y = 0.0L;
    long double sum_xx = 0.0L;
    long double sum_xy = 0.0L;
    std::size_t count = 0;
    const long double denominator = static_cast<long double>(dataset_.size() - 1);

    for (std::size_t i = 0; i < dataset_.size(); ++i) {
        const long double x = static_cast<long double>(dataset_[i] - first_key) /
                              static_cast<long double>(key_span);
        const long double y = static_cast<long double>(i) / denominator;
        ++count;
        const long double delta_x = x - mean_x;
        const long double delta_y = y - mean_y;
        mean_x += delta_x / static_cast<long double>(count);
        mean_y += delta_y / static_cast<long double>(count);
        sum_xx += delta_x * (x - mean_x);
        sum_xy += delta_x * (y - mean_y);
    }

    if (sum_xx > 0.0L) {
        slope_ = sum_xy / sum_xx;
        intercept_ = mean_y - slope_ * mean_x;
    }
}

std::size_t LearnedIndex::predict_position(Key key) const noexcept {
    if (dataset_.empty() || dataset_.size() == 1 ||
        key <= dataset_[0]) {
        return 0;
    }
    const std::size_t last = dataset_.size() - 1;
    if (key >= dataset_[last]) {
        return last;
    }

    const Key key_span = dataset_[last] - dataset_[0];
    const long double normalized_key =
        static_cast<long double>(key - dataset_[0]) /
        static_cast<long double>(key_span);
    const long double raw_position =
        (intercept_ + slope_ * normalized_key) * static_cast<long double>(last);

    if (raw_position <= 0.0L) {
        return 0;
    }
    if (raw_position >= static_cast<long double>(last)) {
        return last;
    }
    return static_cast<std::size_t>(std::floor(raw_position + 0.5L));
}

std::optional<std::size_t> LearnedIndex::find(Key key) const noexcept {
    const std::size_t size = dataset_.size();
    if (size == 0 || key < dataset_[0] || key > dataset_[size - 1]) {
        return std::nullopt;
    }

    const std::size_t predicted = predict_position(key);
    if (dataset_[predicted] == key) {
        return predicted;
    }

    std::size_t position = predicted;
    std::size_t step = 1;
    if (dataset_[predicted] < key) {
        std::size_t first = predicted + 1;
        std::size_t probe = first;
        while (probe < size && dataset_[probe] < key) {
            first = probe + 1;
            if (first == size) {
                return std::nullopt;
            }
            const std::size_t remaining = size - 1 - probe;
            probe += std::min(step, remaining);
            step = step > size / 2 ? size : step * 2;
        }
        position = lower_bound_index(dataset_, first, probe + 1, key);
    } else {
        std::size_t first = predicted;
        std::size_t last = predicted + 1;
        while (first > 0 && dataset_[first - 1] > key) {
            last = first;
            first = first > step ? first - step : 0;
            step = step > size / 2 ? size : step * 2;
        }
        if (first > 0) {
            --first;
        }
        position = lower_bound_index(dataset_, first, last, key);
    }

    if (position < size && dataset_[position] == key) {
        return position;
    }
    return std::nullopt;
}

bool LearnedIndex::contains(Key key) const noexcept {
    return find(key).has_value();
}

} // namespace learned_index
