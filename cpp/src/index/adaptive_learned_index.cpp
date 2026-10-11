#include <learned_index/index/adaptive_learned_index.hpp>

#include <iterator>
#include <mutex>
#include <stdexcept>
#include <utility>
#include <vector>

namespace learned_index {

AdaptiveLearnedIndex::Snapshot::Snapshot(SortedDataset data)
    : dataset(std::move(data)), model(dataset) {}

AdaptiveLearnedIndex::AdaptiveLearnedIndex(
    SortedDataset initial,
    std::size_t rebuild_threshold
)
    : snapshot_(std::make_shared<Snapshot>(std::move(initial))),
      size_(snapshot_->dataset.size()),
      rebuild_threshold_(rebuild_threshold) {
    if (rebuild_threshold_ == 0) {
        throw std::invalid_argument("rebuild_threshold must be greater than zero");
    }
}

bool AdaptiveLearnedIndex::contains(Key key) const {
    const std::shared_lock lock(mutex_);
    return delta_.contains(key) || snapshot_->model.contains(key);
}

bool AdaptiveLearnedIndex::insert(Key key) {
    const std::unique_lock lock(mutex_);
    if (delta_.contains(key) || snapshot_->model.contains(key)) {
        return false;
    }

    delta_.insert(key);
    ++size_;
    if (delta_.size() >= rebuild_threshold_) {
        rebuild();
    }
    return true;
}

void AdaptiveLearnedIndex::rebuild() {
    std::vector<Key> merged;
    merged.reserve(size_);
    std::size_t base_position = 0;
    auto delta_position = delta_.begin();
    while (base_position < snapshot_->dataset.size() || delta_position != delta_.end()) {
        if (delta_position == delta_.end() ||
            (base_position < snapshot_->dataset.size() &&
             snapshot_->dataset[base_position] < *delta_position)) {
            merged.push_back(snapshot_->dataset[base_position++]);
        } else {
            merged.push_back(*delta_position++);
        }
    }

    snapshot_ = std::make_shared<Snapshot>(SortedDataset(std::move(merged)));
    delta_.clear();
    ++rebuild_count_;
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

} // namespace learned_index
