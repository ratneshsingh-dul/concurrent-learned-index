#include <gtest/gtest.h>

#include <learned_index/data/sorted_dataset.hpp>
#include <learned_index/search/binary_search.hpp>

namespace learned_index{

    TEST(BinarySearchTest,FindExistingMiddleKey){
        const SortedDataset dataset({10,25,40,55,70});
        const auto result=find(dataset,40);

        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(*result,2);
        EXPECT_EQ(dataset[*result],40);
    }

    TEST(BinarySearchTest, ReturnsNotFoundForEmptyDataset) {
    const SortedDataset dataset({});

    const auto result = find(dataset, 40);

    EXPECT_FALSE(result.has_value());
}

    TEST(BinarySearchTest, FindsSingleElement) {
    const SortedDataset dataset({40});

    const auto result = find(dataset, 40);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0);
    EXPECT_EQ(dataset[*result], 40);
}
   
    TEST(BinarySearchTest, ReturnsNotFoundForSingleElement) {
    const SortedDataset dataset({40});

    const auto result = find(dataset, 50);

    EXPECT_FALSE(result.has_value());
}

    TEST(BinarySearchTest, FindsFirstKey) {
    const SortedDataset dataset({10, 25, 40, 55, 70});

    const auto result = find(dataset, 10);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0);
    EXPECT_EQ(dataset[*result], 10);
}

    TEST(BinarySearchTest, FindsLastKey) {
    const SortedDataset dataset({10, 25, 40, 55, 70});

    const auto result = find(dataset, 70);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 4);
    EXPECT_EQ(dataset[*result], 70);
}

    TEST(BinarySearchTest, ReturnsNotFoundForKeyBetweenElements) {
    const SortedDataset dataset({10, 25, 40, 55, 70});

    const auto result = find(dataset, 50);

    EXPECT_FALSE(result.has_value());
}

    TEST(BinarySearchTest, ReturnsNotFoundForKeySmallerThanAllElements) {
    const SortedDataset dataset({10, 25, 40, 55, 70});

    const auto result = find(dataset, 5);

    EXPECT_FALSE(result.has_value());
}

    TEST(BinarySearchTest, ReturnsNotFoundForKeyLargerThanAllElements) {
    const SortedDataset dataset({10, 25, 40, 55, 70});

    const auto result = find(dataset, 100);

    EXPECT_FALSE(result.has_value());
}

}