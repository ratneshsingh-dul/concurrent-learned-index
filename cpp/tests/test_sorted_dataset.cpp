#include <gtest/gtest.h>

#include <learned_index/data/sorted_dataset.hpp>

TEST(SortedDatasetTest, AcceptsValidSortedUniqueKeys){
    learned_index::SortedDataset dataset({10,25,40,100});

    EXPECT_EQ(dataset.size(),4);
    EXPECT_FALSE(dataset.empty());
    
    EXPECT_EQ(dataset[0],10);
    EXPECT_EQ(dataset[1],25);
    EXPECT_EQ(dataset[2],40);
    EXPECT_EQ(dataset[3],100);
}

TEST(SortedDatasetTest, RejectsDuplicateKeys) {
    EXPECT_THROW(
        learned_index::SortedDataset({10, 25, 25, 100}),
        std::invalid_argument
    );
}

TEST(SortedDatasetTest, RejectsUnsortedKeys) {
    EXPECT_THROW(
        learned_index::SortedDataset({10, 40, 25, 100}),
        std::invalid_argument
   );
}

TEST(SortedDatasetTest, HandlesEmptyDataset){
    learned_index::SortedDataset dataset({});
    EXPECT_TRUE(dataset.empty());
    EXPECT_EQ(dataset.size(), 0);
}

TEST(SortedDatasetTest, HandlesSingleElementDataset) {
    learned_index::SortedDataset dataset({42});

    EXPECT_FALSE(dataset.empty());
    EXPECT_EQ(dataset.size(), 1);
    EXPECT_EQ(dataset[0], 42);
}