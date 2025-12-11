cmake_minimum_required(VERSION 3.20)

project(CXLMC)

set(default_build_type "Release")
set(c_flags )
set(cxx_flags -std=c++17)
set(c_cxx_flags -Wno-unused-parameter -Wmissing-field-initializers -Wall -Wextra
 -pedantic -Wshadow -O3 -g)

add_compile_options(
  "$<$<COMPILE_LANGUAGE:CXX>:${cxx_flags}>"
  "$<$<COMPILE_LANGUAGE:C>:${c_flags}>"
  "${c_cxx_flags}"
  )

if (CXLMC_CXL_ALLOC_RAND)
    add_compile_definitions(CXL_ALLOC_RAND=1)
endif()

if (CXLMC_GPF)
    add_compile_definitions(GPF_ALWAYS_SUCCEEDS=1)
endif()

enable_language(C CXX)

add_subdirectory(src)

add_executable(init src/init.cc)
include_directories(init PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(init PUBLIC ${CMAKE_DL_LIBS} user allocators model)

add_custom_target(run COMMAND $<TARGET_FILE:init> 16 $<TARGET_FILE:userA> $<TARGET_FILE:userB>)
add_custom_target(run_store COMMAND $<TARGET_FILE:init> 16 $<TARGET_FILE:user_store>)
add_custom_target(run_crash COMMAND $<TARGET_FILE:init> 2 $<TARGET_FILE:user_crashA> $<TARGET_FILE:user_crashB>)
add_custom_target(run_mixedsize COMMAND $<TARGET_FILE:init> 2 $<TARGET_FILE:user_mixedsizeA> $<TARGET_FILE:user_mixedsizeB>)
