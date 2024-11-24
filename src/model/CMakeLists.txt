add_library(model SHARED model.cc scheduler.cc snapshot.cc executor.cc threads.cc sleep.cc pthread.cc
    rfentry.cc thread_memory.cc nodestack.cc mutex.cc condition_variable.cc action.cc real_pthread.cc)
include_directories(model PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(model PUBLIC ${CMAKE_DL_LIBS} -pthread)
