cmake_minimum_required(VERSION 3.20)
project(quantinnium_solver LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

add_library(upsm_daemon_worker STATIC src/upsm_solver_daemon.cpp)
target_include_directories(upsm_daemon_worker PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_options(upsm_daemon_worker PRIVATE -Wall -Wextra -Wpedantic)

add_executable(worker_tests tests/worker_tests.cpp)
target_include_directories(worker_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_options(worker_tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME worker_tests COMMAND worker_tests)

add_executable(avx512_kernel_bench benchmarks/avx512_kernel_bench.cpp)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(avx512_kernel_bench PRIVATE -O3 -Wall -Wextra -Wpedantic)
endif()
