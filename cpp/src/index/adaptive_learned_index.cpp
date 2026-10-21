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
    std::unique_lock lock(mutex_);
    if (delta_.contains(key) || snapshot_->model.contains(key)) {
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
            rebuilt = std::make_shared<Snapshot>(SortedDataset(std::move(merged)));
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

} // namespace learned_index
