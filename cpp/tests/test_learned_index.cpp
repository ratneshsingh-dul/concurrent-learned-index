#include <gtest/gtest.h>

#include <learned_index/learned_index.hpp>

TEST(LearnedIndexSmokeTest,ProjectNameIsCorrect){
    EXPECT_EQ(
        learned_index::project_name(),
        "Concurrent Learned Index"
    );
}