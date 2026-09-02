#include <learned_index/search/binary_search.hpp>

namespace learned_index {

std::optional<std::size_t> find(
    const SortedDataset& dataset,
    Key key
) {
    std::size_t left=0;
    std::size_t right=dataset.size();

    while(left<right){
        const std::size_t mid=left+(right-left)/2;

        if(dataset[mid]==key){
            return mid;
        }

        if(dataset[mid]<key){
            left=mid+1;
        }
        else{
            right=mid;
        }
    }

    return std::nullopt;

}

}