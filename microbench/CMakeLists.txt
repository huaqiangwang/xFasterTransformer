#if (NOT XFT_PERF_BENCH)
#    return()
#endif()

message(STATUS "Build component Performance Bench ...")

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} MICROBENCH_SRC)

add_executable(bench ${MICROBENCH_SRC})
link_directories(${CMAKE_SOURCE_DIR}/xed/lib)
include_directories(${CMAKE_SOURCE_DIR}/xed/include)
target_link_libraries(bench PUBLIC xed)
