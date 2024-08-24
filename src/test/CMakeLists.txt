include_directories(userA PUBLIC "${PROJECT_SOURCE_DIR}/include")
include_directories(userB PUBLIC "${PROJECT_SOURCE_DIR}/include")
include_directories(user_store PUBLIC "${PROJECT_SOURCE_DIR}/include")

add_library(userA MODULE userA.cc) 
target_link_libraries(userA PUBLIC user allocators)

add_library(userB MODULE userB.cc)
target_link_libraries(userB PUBLIC user allocators)

add_library(user_store MODULE user_store.cc)
target_link_libraries(user_store PUBLIC user allocators)
