#pragma once

#include <cstddef>
#include <optional>

#include <learned_index/data/key.hpp>
#include <learned_index/data/sorted_dataset.hpp>

namespace learned_index{

    class BinarySearchIndex{
        private:
        const SortedDataset& dataset_;

        public:
        explicit BinarySearchIndex(
            const SortedDataset& dataset
        );

        [[nodiscard]] std::optional<std::size_t> find(
            Key key
        ) const;

        [[nodiscard]] bool contains(Key key) const;

    };
}
