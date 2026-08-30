#include <stdexcept>
#include <cstddef>
#include <utility>

#include <learned_index/data/sorted_dataset.hpp>

namespace learned_index{

    SortedDataset::SortedDataset(std::vector<Key> keys): keys_(std::move(keys)){
        for(std::size_t i=1;i<keys_.size();++i){
            if(keys_[i]<=keys_[i-1]){
                throw std::invalid_argument(
                    "Keys must be sorted in strictly increasing order"
                );
            }
        }
    }

    std::size_t SortedDataset::size() const noexcept {
        return keys_.size();
    }

    bool SortedDataset::empty() const noexcept{
        return keys_.empty();
    }

    Key SortedDataset::operator[](std::size_t index) const {
        return keys_[index];
    }

}