add_library(user SHARED user.cc)
include_directories(user PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(user PUBLIC ${CMAKE_DL_LIBS} model)
