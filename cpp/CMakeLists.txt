add_library(learned_index
     src/learned_index.cpp
)

target_include_directories(learned_index
   PUBLIC
       ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(learned_index_app
    apps/main.cpp
)

target_link_libraries(learned_index_app
   PRIVATE
     learned_index
)

add_executable(learned_index_tests
    tests/test_learned_index.cpp
)

target_link_libraries(learned_index_tests
     PRIVATE
        learned_index
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(learned_index_tests)

