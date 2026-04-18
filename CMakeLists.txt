cmake_minimum_required(VERSION 3.15)
project(MatchingEngine VERSION 1.0 LANGUAGES CXX)

# --- Standard & Strictness ---
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_COMPILE_WARNING_AS_ERROR ON) # Treat all warnings as errors

# --- Compiler Warnings ---
add_library(project_warnings INTERFACE)
target_compile_options(project_warnings INTERFACE
    -Wall -Wextra -Wpedantic -Wshadow -Wnon-virtual-dtor
    -Wold-style-cast -Wunused -Woverloaded-virtual -Wconversion
)

# --- Clang-Tidy Integration ---
find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
if(CLANG_TIDY_EXE)
    set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_EXE} "--extra-arg=-Wno-unknown-warning-option")
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
# add_subdirectory(engine)
# add_subdirectory(physics)

# --- Main Executable ---
add_executable(app main.cpp)
target_link_libraries(app PRIVATE project_warnings)
