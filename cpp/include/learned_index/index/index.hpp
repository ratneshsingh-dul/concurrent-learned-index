#pragma once
#include <concepts>
#include <learned_index/data/key.hpp>

namespace learned_index{

    template<typename T>
    concept Index=requires(const T& index,Key key){
        {
            index.contains(key)
        } -> std::same_as<bool>;
    };
}
