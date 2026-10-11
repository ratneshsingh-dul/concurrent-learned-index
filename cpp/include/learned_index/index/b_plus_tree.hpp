#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

#include <learned_index/data/key.hpp>
#include <learned_index/data/sorted_dataset.hpp>

namespace learned_index {

// An in-memory B+ tree for unique uint64 keys. Leaves own keys; internal
// separator keys are copies of the minimum key in each child after child 0.
class BPlusTree {
private:
    struct Node {
        explicit Node(bool is_leaf) : leaf(is_leaf) {}

        bool leaf;
        std::vector<Key> keys;
        std::vector<std::unique_ptr<Node>> children;
        Node* next_leaf = nullptr; // Non-owning; nodes are tree-owned.
    };

    struct Split {
        Key separator;
        std::unique_ptr<Node> right;
    };

    struct InsertResult {
        bool inserted;
        std::optional<Split> split;
    };

    std::size_t max_keys_;
    std::size_t size_ = 0;
    std::unique_ptr<Node> root_;

    [[nodiscard]] static Key subtree_min(const Node& node);
    [[nodiscard]] std::unique_ptr<Node> make_node(bool leaf) const;
    [[nodiscard]] std::unique_ptr<Node> make_root_leaf() const;
    [[nodiscard]] std::unique_ptr<Node> make_root_internal(
        std::vector<std::unique_ptr<Node>> children
    ) const;
    [[nodiscard]] std::unique_ptr<Node> build_internal_level(
        std::vector<std::unique_ptr<Node>> children
    ) const;
    [[nodiscard]] Split split_leaf(Node& leaf) const;
    [[nodiscard]] Split split_internal(Node& node) const;
    [[nodiscard]] InsertResult insert_recursive(Node& node, Key key);

public:
    // max_keys is the maximum number of keys in a node and must be at least 3.
    explicit BPlusTree(std::size_t max_keys = 32);
    BPlusTree(BPlusTree&&) noexcept = default;
    BPlusTree& operator=(BPlusTree&&) noexcept = default;
    BPlusTree(const BPlusTree&) = delete;
    BPlusTree& operator=(const BPlusTree&) = delete;

    // Builds a balanced tree from an already sorted, unique dataset in O(n).
    [[nodiscard]] static BPlusTree bulk_load(
        const SortedDataset& dataset,
        std::size_t max_keys = 32
    );

    // Inserts a new key. Returns false and leaves the tree unchanged on duplicate.
    [[nodiscard]] bool insert(Key key);
    [[nodiscard]] bool contains(Key key) const;
    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] bool empty() const noexcept;

    // Returns keys in the inclusive range [lower, upper].
    [[nodiscard]] std::vector<Key> range_query(Key lower, Key upper) const;

    // Checks ordering, occupancy, separators, leaf depth/link order, and size.
    [[nodiscard]] bool validate() const;
};

} // namespace learned_index
