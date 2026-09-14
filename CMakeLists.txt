cmake_minimum_required(VERSION 3.20)
project(tengine VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(TENGINE_BUILD_TESTS "Build T-Engine tests" ON)

add_library(tengine_runtime STATIC
    runtime/t_runtime.c
)
target_include_directories(tengine_runtime PUBLIC runtime)
target_compile_options(tengine_runtime PRIVATE
    $<$<C_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
)

add_library(tengine_frontend STATIC
    src/lexer.cpp
    src/parser.cpp
    src/interpreter.cpp
    src/type_checker.cpp
    src/bytecode.cpp
    src/builtins.cpp
)
target_include_directories(tengine_frontend PUBLIC include)
target_compile_options(tengine_frontend PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
)

add_executable(tengine src/main.cpp)
target_link_libraries(tengine PRIVATE tengine_frontend tengine_runtime)

if(TENGINE_BUILD_TESTS)
    enable_testing()
    add_executable(tengine_tests tests/test_frontend.cpp)
    target_link_libraries(tengine_tests PRIVATE tengine_frontend tengine_runtime)
    add_test(NAME tengine_frontend COMMAND tengine_tests)
endif()
