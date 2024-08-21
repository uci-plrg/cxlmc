add_library(threads SHARED real_pthread.cc)
include_directories(threads PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(threads PUBLIC ${CMAKE_DL_LIBS} -pthread)