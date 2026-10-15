#include <iostream>

#include <learned_index/data/sorted_dataset.hpp>
#include <learned_index/index/adaptive_learned_index.hpp>
#include <learned_index/index/b_plus_tree.hpp>
#include <learned_index/index/binary_search_index.hpp>
#include <learned_index/index/learned_index.hpp>
#include <learned_index/index/rmi_learned_index.hpp>
#include <learned_index/learned_index.hpp>

int main() {
    std::cout << learned_index::project_name() << "\n\n";

    const learned_index::SortedDataset dataset({10, 25, 40, 55, 70});
    const learned_index::BinarySearchIndex binary(dataset);
    const auto tree = learned_index::BPlusTree::bulk_load(dataset);
    const learned_index::LearnedIndex learned(dataset);
    const learned_index::RmiLearnedIndex rmi(dataset);

    std::cout << "Static indexes (query: 40)\n";
    std::cout << "  binary search position: ";
    if (const auto position = binary.find(40)) {
        std::cout << *position << '\n';
    } else {
        std::cout << "not found\n";
    }
    std::cout << "  B+ tree contains: " << std::boolalpha << tree.contains(40) << '\n';
    std::cout << "  learned index contains: " << learned.contains(40) << '\n';
    std::cout << "  RMI learned index contains: " << rmi.contains(40) << '\n';

    learned_index::AdaptiveLearnedIndex adaptive(dataset, 2);
    std::cout << "\nAdaptive index\n";
    std::cout << "  insert 42: " << adaptive.insert(42) << '\n';
    std::cout << "  insert 54: " << adaptive.insert(54) << '\n';
    std::cout << "  contains 54 after delta merge/retrain: " << adaptive.contains(54) << '\n';
    std::cout << "  rebuild count: " << adaptive.rebuild_count() << '\n';
    return 0;
}
