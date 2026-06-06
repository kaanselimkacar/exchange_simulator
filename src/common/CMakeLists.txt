add_library(common STATIC
    spdlog_logger.cpp
)

target_include_directories(common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(common SYSTEM PUBLIC
    ${CMAKE_SOURCE_DIR}/third_party/spdlog/include
)

# Add compile-time DEBUG_BUILD flag for debug builds
target_compile_definitions(common PUBLIC
    $<$<CONFIG:Debug>:DEBUG_BUILD>
)

target_link_libraries(common PRIVATE project_warnings)
