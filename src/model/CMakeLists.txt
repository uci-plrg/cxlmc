add_library(model SHARED model.cc scheduler.cc snapshot.cc)
include_directories(model PUBLIC "${PROJECT_SOURCE_DIR}/include")
