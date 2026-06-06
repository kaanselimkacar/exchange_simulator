cmake_minimum_required(VERSION 3.15)
project(MatchingEngine VERSION 1.0 LANGUAGES CXX)

# --- Standard & Strictness ---
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_COMPILE_WARNING_AS_ERROR ON) # Treat all warnings as errors
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Compiler Warnings ---
add_library(project_warnings INTERFACE)
target_compile_options(project_warnings INTERFACE
    -Wall -Wextra -Wpedantic -Wshadow -Wnon-virtual-dtor
    -Wold-style-cast -Wunused -Woverloaded-virtual -Wconversion
)

# --- Clang-Tidy Integration ---
find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
if(CLANG_TIDY_EXE)
    if(DEFINED ENV{NIX_CFLAGS_COMPILE})
        separate_arguments(NIX_FLAGS NATIVE_COMMAND "$ENV{NIX_CFLAGS_COMPILE}")
        set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_EXE})
        foreach(flag ${NIX_FLAGS})
            list(APPEND CMAKE_CXX_CLANG_TIDY "--extra-arg-before=${flag}")
        endforeach()
    else()
        set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_EXE})
    endif()
endif()

# --- Clang-Format Target ---
find_program(CLANG_FORMAT_EXE NAMES "clang-format")
if(CLANG_FORMAT_EXE)
    file(GLOB_RECURSE ALL_SOURCE_FILES *.cpp *.hpp *.h)
    add_custom_target(format
        COMMAND ${CLANG_FORMAT_EXE} -i -style=file ${ALL_SOURCE_FILES}
    )
endif()

# --- Subdirectories (Logic Folders) ---
# Add your folders here as you create them
add_subdirectory(src)

# --- Testing ---
add_subdirectory(tests)

# --- Main Executable ---
add_executable(app main.cpp)
target_link_libraries(app PRIVATE project_warnings)
target_link_libraries(app PRIVATE src)

# --- Build and run tests for Debug builds ---
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_custom_target(build_and_test ALL
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target market_tests
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target run_tests
        DEPENDS market_tests
        COMMENT "Building and running tests in Debug mode..."
    )
endif()
