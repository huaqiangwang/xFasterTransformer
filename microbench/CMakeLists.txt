#if (NOT XFT_PERF_BENCH)
#    return()
#endif()

message(STATUS "Build Component Performance Bench ...")

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} MICROBENCH_SRC)

add_executable(bench ${MICROBENCH_SRC})

include_directories(${CMAKE_SOURCE_DIR}/microbench/xed/include)

target_link_libraries(bench PRIVATE -lstdc++ -lstdc++fs xed xfastertransformer)

