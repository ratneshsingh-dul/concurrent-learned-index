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
