#include <learned_index/index/adaptive_learned_index.hpp>

#include <algorithm>
#include <iterator>
#include <mutex>
#include <stdexcept>
#include <utility>
#include <vector>

namespace learned_index {
namespace {

using ModelVariant = std::variant<LearnedIndex, RmiLearnedIndex>;

long double mean_sampled_rank_error(
    const SortedDataset& dataset,
    const LearnedIndex& model
) {
    if (dataset.empty()) {
        return 0.0L;
    }
    constexpr std::size_t max_samples = 256;
    const std::size_t sample_count = std::min(dataset.size(), max_samples);
    long double total_error = 0.0L;
    for (std::size_t i = 0; i < sample_count; ++i) {
        const std::size_t position = sample_count == 1
            ? 0
            : ((dataset.size() - 1) / (sample_count - 1)) * i +
                (((dataset.size() - 1) % (sample_count - 1)) * i) /
                    (sample_count - 1);
        const std::size_t predicted = model.predict_position(dataset[position]);
        total_error += predicted > position
            ? static_cast<long double>(predicted - position)
            : static_cast<long double>(position - predicted);
    }
    return total_error / static_cast<long double>(sample_count);
}

long double mean_sampled_rank_error(
    const SortedDataset& dataset,
    const RmiLearnedIndex& model
) {
    if (dataset.empty()) {
        return 0.0L;
    }
    constexpr std::size_t max_samples = 256;
    const std::size_t sample_count = std::min(dataset.size(), max_samples);
    long double total_error = 0.0L;
    for (std::size_t i = 0; i < sample_count; ++i) {
        const std::size_t position = sample_count == 1
            ? 0
            : ((dataset.size() - 1) / (sample_count - 1)) * i +
                (((dataset.size() - 1) % (sample_count - 1)) * i) /
                    (sample_count - 1);
        const std::size_t predicted = model.predict_position(dataset[position]);
        total_error += predicted > position
            ? static_cast<long double>(predicted - position)
            : static_cast<long double>(position - predicted);
    }
    return total_error / static_cast<long double>(sample_count);
}

ModelVariant train_model(
    const SortedDataset& dataset,
    AdaptiveLearnedIndex::ModelKind policy,
    std::size_t rmi_leaf_size
) {
    if (policy == AdaptiveLearnedIndex::ModelKind::rmi) {
        return ModelVariant(std::in_place_type<RmiLearnedIndex>, dataset, rmi_leaf_size);
    }
    if (policy == AdaptiveLearnedIndex::ModelKind::global) {
        return ModelVariant(std::in_place_type<LearnedIndex>, dataset);
    }

    LearnedIndex global_model(dataset);
    RmiLearnedIndex rmi_model(dataset, rmi_leaf_size);
    const long double global_error = mean_sampled_rank_error(dataset, global_model);
    const long double rmi_error = mean_sampled_rank_error(dataset, rmi_model);
    constexpr long double required_error_reduction = 0.20L;
    if (global_error > 0.0L &&
        rmi_error <= global_error * (1.0L - required_error_reduction)) {
        return ModelVariant(std::move(rmi_model));
    }
    return ModelVariant(std::move(global_model));
}

} // namespace

AdaptiveLearnedIndex::Snapshot::Snapshot(
    SortedDataset data,
    ModelKind model_kind,
    std::size_t rmi_leaf_size
)
    : dataset(std::move(data)),
      model(train_model(dataset, model_kind, rmi_leaf_size)),
      selected_model_kind(std::holds_alternative<RmiLearnedIndex>(model)
          ? ModelKind::rmi
          : ModelKind::global) {}

AdaptiveLearnedIndex::AdaptiveLearnedIndex(
    SortedDataset initial,
    std::size_t rebuild_threshold,
    ModelKind model_kind,
    std::size_t rmi_leaf_size
)
    : snapshot_(std::make_shared<Snapshot>(
          std::move(initial), model_kind, rmi_leaf_size
      )),
      size_(snapshot_->dataset.size()),
      rebuild_threshold_(rebuild_threshold),
      model_policy_(model_kind),
      rmi_leaf_size_(rmi_leaf_size) {
    if (rebuild_threshold_ == 0) {
        throw std::invalid_argument("rebuild_threshold must be greater than zero");
    }
    if (model_policy_ != ModelKind::global && rmi_leaf_size_ == 0) {
        throw std::invalid_argument("rmi_leaf_size must be greater than zero");
    }
}

bool AdaptiveLearnedIndex::contains(Key key) const {
    const std::shared_lock lock(mutex_);
    return delta_.contains(key) || std::visit(
        [key](const auto& model) { return model.contains(key); }, snapshot_->model
    );
}

bool AdaptiveLearnedIndex::insert(Key key) {
    std::unique_lock lock(mutex_);
    if (delta_.contains(key) || std::visit(
            [key](const auto& model) { return model.contains(key); }, snapshot_->model
        )) {
        return false;
    }

    delta_.insert(key);
    ++size_;
    if (delta_.size() < rebuild_threshold_ || rebuild_in_progress_) {
        return true;
    }

    rebuild_in_progress_ = true;
    while (true) {
        const auto base = snapshot_;
        std::vector<Key> captured_delta;
        try {
            captured_delta.assign(delta_.begin(), delta_.end());
        } catch (...) {
            rebuild_in_progress_ = false;
            throw;
        }
        lock.unlock();

        std::shared_ptr<const Snapshot> rebuilt;
        try {
            std::vector<Key> merged;
            merged.reserve(base->dataset.size() + captured_delta.size());
            std::size_t base_position = 0;
            auto delta_position = captured_delta.begin();
            while (base_position < base->dataset.size() ||
                   delta_position != captured_delta.end()) {
                if (delta_position == captured_delta.end() ||
                    (base_position < base->dataset.size() &&
                     base->dataset[base_position] < *delta_position)) {
                    merged.push_back(base->dataset[base_position++]);
                } else {
                    merged.push_back(*delta_position++);
                }
            }
            rebuilt = std::make_shared<Snapshot>(
                SortedDataset(std::move(merged)), model_policy_, rmi_leaf_size_
            );
        } catch (...) {
            lock.lock();
            rebuild_in_progress_ = false;
            throw;
        }

        lock.lock();
        snapshot_ = std::move(rebuilt);
        for (const Key captured_key : captured_delta) {
            delta_.erase(captured_key);
        }
        ++rebuild_count_;

        if (delta_.size() < rebuild_threshold_) {
            rebuild_in_progress_ = false;
            return true;
        }
    }
}

std::size_t AdaptiveLearnedIndex::size() const {
    const std::shared_lock lock(mutex_);
    return size_;
}

std::size_t AdaptiveLearnedIndex::pending_size() const {
    const std::shared_lock lock(mutex_);
    return delta_.size();
}

std::size_t AdaptiveLearnedIndex::rebuild_count() const {
    const std::shared_lock lock(mutex_);
    return rebuild_count_;
}

AdaptiveLearnedIndex::ModelKind AdaptiveLearnedIndex::selected_model_kind() const {
    const std::shared_lock lock(mutex_);
    return snapshot_->selected_model_kind;
}

} // namespace learned_index
