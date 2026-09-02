#include <iostream>
#include <learned_index/data/sorted_dataset.hpp>
#include <learned_index/learned_index.hpp>
#include <learned_index/search/binary_search.hpp>

int main(){
    std::cout<<learned_index::project_name()<<'\n';

    const learned_index::SortedDataset dataset(
        {10,25,40,55,70}
    );

    const auto found=learned_index::find(dataset,40);

    if(found.has_value()){
        std::cout<<"Found 40 at index: "<<*found <<'\n';
    }
    else{
        std::cout<<"40 not found\n";
    }

    const auto missing=learned_index::find(dataset,50);

    if(missing.has_value()){
        std::cout<<"Found 50 at index: "<<*missing<<'\n';
    }
    else{
        std::cout<<"50 not found\n";
    }
    
    return 0;

    }