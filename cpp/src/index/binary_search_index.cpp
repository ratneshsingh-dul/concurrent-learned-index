#include <learned_index/index/binary_search_index.hpp>
#include <learned_index/search/binary_search.hpp>

namespace learned_index{

    BinarySearchIndex::BinarySearchIndex(
        const SortedDataset& dataset
    )
       : dataset_(dataset)
       {
       }

    std::optional<std::size_t> BinarySearchIndex::find(
        Key key
    ) const{
        return learned_index::find(dataset_,key);
    }

    bool BinarySearchIndex::contains(Key key) const{
        return find(key).has_value();
    }
}
