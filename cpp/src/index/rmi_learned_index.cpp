#include <learned_index/index/rmi_learned_index.hpp>

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

RmiLearnedIndex::RmiLearnedIndex(
    const SortedDataset& dataset,
    std::size_t target_leaf_size
)
    : dataset_(dataset), target_leaf_size_(target_leaf_size) {
    if (target_leaf_size_ == 0) {
        throw std::invalid_argument("target_leaf_size must be greater than zero");
    }
    train();
}

RmiLearnedIndex::LinearModel RmiLearnedIndex::fit_model(
    const SortedDataset& dataset,
    std::size_t first_position,
    std::size_t last_position
) noexcept {
    const Key first_key = dataset[first_position];
    const Key last_key = dataset[last_position];
    const Key key_span = last_key - first_key;
    const std::size_t rank_span = last_position - first_position;
    long double intercept = 0.0L;
    long double slope = 0.0L;

    if (rank_span != 0 && key_span != 0) {
        long double mean_x = 0.0L;
        long double mean_y = 0.0L;
        long double sum_xx = 0.0L;
        long double sum_xy = 0.0L;
        std::size_t count = 0;
        for (std::size_t position = first_position;
             position <= last_position; ++position) {
            const long double x = static_cast<long double>(
                dataset[position] - first_key
            ) / static_cast<long double>(key_span);
            const long double y = static_cast<long double>(position - first_position) /
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

    return LinearModel{
        first_position, last_position, first_key, last_key, intercept, slope
    };
}

void RmiLearnedIndex::train() {
    leaves_.clear();
    if (dataset_.empty()) {
        return;
    }

    root_ = fit_model(dataset_, 0, dataset_.size() - 1);
    const std::size_t leaf_count = dataset_.size() / target_leaf_size_ +
        (dataset_.size() % target_leaf_size_ != 0);
    leaves_.reserve(leaf_count);
    for (std::size_t first = 0; first < dataset_.size();) {
        const std::size_t width = std::min(target_leaf_size_, dataset_.size() - first);
        const std::size_t last = first + width - 1;
        leaves_.push_back(fit_model(dataset_, first, last));
        first += width;
    }
}

std::size_t RmiLearnedIndex::predict(
    const LinearModel& model,
    Key key
) noexcept {
    if (model.first_position == model.last_position || key <= model.first_key) {
        return model.first_position;
    }
    if (key >= model.last_key) {
        return model.last_position;
    }

    const Key key_span = model.last_key - model.first_key;
    const std::size_t rank_span = model.last_position - model.first_position;
    const long double normalized_key = static_cast<long double>(
        key - model.first_key
    ) / static_cast<long double>(key_span);
    const long double raw_local_rank =
        (model.intercept + model.slope * normalized_key) *
        static_cast<long double>(rank_span);
    if (raw_local_rank <= 0.0L) {
        return model.first_position;
    }
    if (raw_local_rank >= static_cast<long double>(rank_span)) {
        return model.last_position;
    }
    return model.first_position + static_cast<std::size_t>(
        std::floor(raw_local_rank + 0.5L)
    );
}

std::size_t RmiLearnedIndex::predict_position(Key key) const noexcept {
    if (dataset_.empty() || key <= dataset_[0]) {
        return 0;
    }
    const std::size_t last_position = dataset_.size() - 1;
    if (key >= dataset_[last_position]) {
        return last_position;
    }

    const std::size_t root_prediction = predict(root_, key);
    const std::size_t leaf_index = std::min(
        root_prediction / target_leaf_size_, leaves_.size() - 1
    );
    return predict(leaves_[leaf_index], key);
}

std::optional<std::size_t> RmiLearnedIndex::find(Key key) const noexcept {
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

bool RmiLearnedIndex::contains(Key key) const noexcept {
    return find(key).has_value();
}

} // namespace learned_index
