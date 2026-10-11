#include <learned_index/index/b_plus_tree.hpp>

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace learned_index {
namespace {

std::size_t ceil_div(std::size_t numerator, std::size_t denominator) {
    return numerator / denominator + (numerator % denominator != 0);
}

} // namespace

BPlusTree::BPlusTree(std::size_t max_keys)
    : max_keys_(max_keys), root_(std::make_unique<Node>(true)) {
    if (max_keys_ < 3) {
        throw std::invalid_argument("BPlusTree max_keys must be at least 3");
    }
}

Key BPlusTree::subtree_min(const Node& node) {
    const Node* current = &node;
    while (!current->leaf) {
        current = current->children.front().get();
    }
    return current->keys.front();
}

std::unique_ptr<BPlusTree::Node> BPlusTree::make_node(bool leaf) const {
    auto node = std::make_unique<Node>(leaf);
    node->keys.reserve(max_keys_ + 1);
    if (!leaf) {
        node->children.reserve(max_keys_ + 2);
    }
    return node;
}

std::unique_ptr<BPlusTree::Node> BPlusTree::make_root_leaf() const {
    return make_node(true);
}

std::unique_ptr<BPlusTree::Node> BPlusTree::make_root_internal(
    std::vector<std::unique_ptr<Node>> children
) const {
    auto root = make_node(false);
    for (auto& child : children) {
        root->children.push_back(std::move(child));
    }
    root->keys.reserve(root->children.size() - 1);
    for (std::size_t i = 1; i < root->children.size(); ++i) {
        root->keys.push_back(subtree_min(*root->children[i]));
    }
    return root;
}

std::unique_ptr<BPlusTree::Node> BPlusTree::build_internal_level(
    std::vector<std::unique_ptr<Node>> children
) const {
    const std::size_t max_children = max_keys_ + 1;
    const std::size_t parent_count = ceil_div(children.size(), max_children);
    const std::size_t base_children = children.size() / parent_count;
    const std::size_t extra_children = children.size() % parent_count;

    std::vector<std::unique_ptr<Node>> parents;
    parents.reserve(parent_count);

    std::size_t offset = 0;
    for (std::size_t i = 0; i < parent_count; ++i) {
        const std::size_t child_count = base_children + (i < extra_children ? 1 : 0);
        std::vector<std::unique_ptr<Node>> group;
        group.reserve(child_count);
        for (std::size_t j = 0; j < child_count; ++j) {
            group.push_back(std::move(children[offset++]));
        }
        parents.push_back(make_root_internal(std::move(group)));
    }

    if (parents.size() == 1) {
        return std::move(parents.front());
    }
    return build_internal_level(std::move(parents));
}

BPlusTree BPlusTree::bulk_load(const SortedDataset& dataset, std::size_t max_keys) {
    BPlusTree tree(max_keys);
    tree.size_ = dataset.size();
    if (dataset.empty()) {
        return tree;
    }

    const std::size_t leaf_count = ceil_div(dataset.size(), max_keys);
    const std::size_t base_keys = dataset.size() / leaf_count;
    const std::size_t extra_keys = dataset.size() % leaf_count;
    std::vector<std::unique_ptr<Node>> level;
    level.reserve(leaf_count);

    std::size_t offset = 0;
    Node* previous_leaf = nullptr;
    for (std::size_t i = 0; i < leaf_count; ++i) {
        const std::size_t count = base_keys + (i < extra_keys ? 1 : 0);
        auto leaf = tree.make_node(true);
        leaf->keys.reserve(count);
        for (std::size_t j = 0; j < count; ++j) {
            leaf->keys.push_back(dataset[offset++]);
        }
        if (previous_leaf != nullptr) {
            previous_leaf->next_leaf = leaf.get();
        }
        previous_leaf = leaf.get();
        level.push_back(std::move(leaf));
    }

    if (level.size() == 1) {
        tree.root_ = std::move(level.front());
    } else {
        tree.root_ = tree.build_internal_level(std::move(level));
    }
    return tree;
}

BPlusTree::Split BPlusTree::split_leaf(Node& leaf) const {
    auto right = make_node(true);
    const std::size_t middle = leaf.keys.size() / 2;
    right->keys.assign(leaf.keys.begin() + static_cast<std::ptrdiff_t>(middle),
                       leaf.keys.end());
    leaf.keys.erase(leaf.keys.begin() + static_cast<std::ptrdiff_t>(middle),
                    leaf.keys.end());
    right->next_leaf = leaf.next_leaf;
    leaf.next_leaf = right.get();
    return Split{right->keys.front(), std::move(right)};
}

BPlusTree::Split BPlusTree::split_internal(Node& node) const {
    auto right = make_node(false);
    const std::size_t middle = node.keys.size() / 2;
    const Key separator = node.keys[middle];

    right->keys.assign(
        node.keys.begin() + static_cast<std::ptrdiff_t>(middle + 1),
        node.keys.end()
    );
    node.keys.erase(node.keys.begin() + static_cast<std::ptrdiff_t>(middle),
                    node.keys.end());

    for (std::size_t i = middle + 1; i < node.children.size(); ++i) {
        right->children.push_back(std::move(node.children[i]));
    }
    node.children.resize(middle + 1);
    return Split{separator, std::move(right)};
}

BPlusTree::InsertResult BPlusTree::insert_recursive(Node& node, Key key) {
    if (node.leaf) {
        const auto position = std::lower_bound(node.keys.begin(), node.keys.end(), key);
        if (position != node.keys.end() && *position == key) {
            return {false, std::nullopt};
        }
        node.keys.insert(position, key);
        if (node.keys.size() > max_keys_) {
            return {true, split_leaf(node)};
        }
        return {true, std::nullopt};
    }

    const auto position = std::upper_bound(node.keys.begin(), node.keys.end(), key);
    const std::size_t child_index =
        static_cast<std::size_t>(position - node.keys.begin());
    auto result = insert_recursive(*node.children[child_index], key);
    if (!result.inserted) {
        return result;
    }

    if (child_index > 0) {
        node.keys[child_index - 1] = subtree_min(*node.children[child_index]);
    }
    if (result.split) {
        node.keys.insert(
            node.keys.begin() + static_cast<std::ptrdiff_t>(child_index),
            result.split->separator
        );
        node.children.insert(
            node.children.begin() + static_cast<std::ptrdiff_t>(child_index + 1),
            std::move(result.split->right)
        );
        result.split.reset();
    }

    if (node.keys.size() > max_keys_) {
        result.split = split_internal(node);
    }
    return result;
}

bool BPlusTree::insert(Key key) {
    auto result = insert_recursive(*root_, key);
    if (!result.inserted) {
        return false;
    }

    if (result.split) {
        std::vector<std::unique_ptr<Node>> children;
        children.push_back(std::move(root_));
        children.push_back(std::move(result.split->right));
        root_ = make_root_internal(std::move(children));
    }
    ++size_;
    return true;
}

bool BPlusTree::contains(Key key) const {
    const Node* node = root_.get();
    while (!node->leaf) {
        const auto position = std::upper_bound(node->keys.begin(), node->keys.end(), key);
        node = node->children[
            static_cast<std::size_t>(position - node->keys.begin())
        ].get();
    }
    const auto position = std::lower_bound(node->keys.begin(), node->keys.end(), key);
    return position != node->keys.end() && *position == key;
}

std::size_t BPlusTree::size() const noexcept {
    return size_;
}

bool BPlusTree::empty() const noexcept {
    return size_ == 0;
}

std::vector<Key> BPlusTree::range_query(Key lower, Key upper) const {
    std::vector<Key> result;
    if (lower > upper || empty()) {
        return result;
    }

    const Node* leaf = root_.get();
    while (!leaf->leaf) {
        const auto position = std::upper_bound(leaf->keys.begin(), leaf->keys.end(), lower);
        leaf = leaf->children[
            static_cast<std::size_t>(position - leaf->keys.begin())
        ].get();
    }

    auto position = std::lower_bound(leaf->keys.begin(), leaf->keys.end(), lower);
    while (leaf != nullptr) {
        for (; position != leaf->keys.end() && *position <= upper; ++position) {
            result.push_back(*position);
        }
        if (position != leaf->keys.end()) {
            break;
        }
        leaf = leaf->next_leaf;
        if (leaf != nullptr) {
            position = leaf->keys.begin();
        }
    }
    return result;
}

bool BPlusTree::validate() const {
    if (root_ == nullptr || max_keys_ < 3) {
        return false;
    }

    struct Summary {
        bool valid;
        std::size_t count;
        std::size_t leaf_depth;
        Key minimum;
        Key maximum;
        bool has_keys;
    };

    std::vector<const Node*> leaves;
    const std::size_t min_leaf_keys = (max_keys_ + 1) / 2;
    const std::size_t min_internal_children = (max_keys_ + 2) / 2;

    const auto inspect = [&](const auto& self,
                             const Node& node,
                             bool is_root,
                             std::size_t depth) -> Summary {
        if (node.keys.size() > max_keys_ ||
            !std::is_sorted(node.keys.begin(), node.keys.end()) ||
            std::adjacent_find(node.keys.begin(), node.keys.end()) != node.keys.end()) {
            return {false, 0, depth, 0, 0, false};
        }

        if (node.leaf) {
            if (!node.children.empty() ||
                (!is_root && node.keys.size() < min_leaf_keys)) {
                return {false, 0, depth, 0, 0, false};
            }
            leaves.push_back(&node);
            if (node.keys.empty()) {
                return {is_root && size_ == 0, 0, depth, 0, 0, false};
            }
            return {true, node.keys.size(), depth, node.keys.front(),
                    node.keys.back(), true};
        }

        if (node.children.size() != node.keys.size() + 1 ||
            node.children.size() > max_keys_ + 1 ||
            (is_root && node.children.size() < 2) ||
            (!is_root && node.children.size() < min_internal_children)) {
            return {false, 0, depth, 0, 0, false};
        }

        std::size_t count = 0;
        std::size_t leaf_depth = 0;
        Key minimum = 0;
        Key maximum = 0;
        bool have_range = false;
        Summary previous{true, 0, 0, 0, 0, false};
        for (std::size_t i = 0; i < node.children.size(); ++i) {
            if (!node.children[i]) {
                return {false, 0, depth, 0, 0, false};
            }
            const Summary child = self(self, *node.children[i], false, depth + 1);
            if (!child.valid || !child.has_keys) {
                return {false, 0, depth, 0, 0, false};
            }
            if (i == 0) {
                minimum = child.minimum;
                leaf_depth = child.leaf_depth;
            } else if (child.leaf_depth != leaf_depth ||
                       previous.maximum >= child.minimum ||
                       node.keys[i - 1] != child.minimum) {
                return {false, 0, depth, 0, 0, false};
            }
            maximum = child.maximum;
            count += child.count;
            previous = child;
            have_range = true;
        }
        return {have_range, count, leaf_depth, minimum, maximum, have_range};
    };

    const Summary summary = inspect(inspect, *root_, true, 0);
    if (!summary.valid || summary.count != size_ || leaves.empty()) {
        return false;
    }
    for (std::size_t i = 0; i < leaves.size(); ++i) {
        const Node* expected_next = (i + 1 < leaves.size()) ? leaves[i + 1] : nullptr;
        if (leaves[i]->next_leaf != expected_next) {
            return false;
        }
    }
    return true;
}

} // namespace learned_index
