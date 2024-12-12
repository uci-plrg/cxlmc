set_source_files_properties(malloc.c PROPERTIES COMPILE_DEFINITIONS "ONLY_MSPACES;HAVE_MMAP=0")

add_library(allocators SHARED malloc.c)
include_directories(allocators PUBLIC "${PROJECT_SOURCE_DIR}/include")
