include_directories(userA PUBLIC "${PROJECT_SOURCE_DIR}/include")
include_directories(userB PUBLIC "${PROJECT_SOURCE_DIR}/include")

add_library(userA MODULE userA.cc) 
target_link_libraries(userA PUBLIC allocators model user)

add_library(userB MODULE userB.cc)
target_link_libraries(userB PUBLIC allocators model user)