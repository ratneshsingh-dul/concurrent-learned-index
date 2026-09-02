#pragma once

#include <cstddef>
#include <optional>

#include <learned_index/data/key.hpp>
#include <learned_index/data/sorted_dataset.hpp>

namespace learned_index{

    [[nodiscard]]
    std::optional<std::size_t> find(
        const SortedDataset& dataset,
        Key key
    );
}