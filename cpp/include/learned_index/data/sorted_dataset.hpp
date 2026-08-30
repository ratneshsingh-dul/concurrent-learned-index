#pragma once

#include <cstddef>
#include <vector>

#include <learned_index/data/key.hpp>

namespace learned_index{

    class SortedDataset{
        private:
        std::vector<Key> keys_;

        public:
        explicit SortedDataset(std::vector<Key> keys);

        [[nodiscard]] std::size_t size() const noexcept;
        [[nodiscard]] bool empty() const noexcept;
        [[nodiscard]] Key operator[](std::size_t index) const;
    };
}