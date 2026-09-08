# Tests Configuration
enable_testing()

# Find Google Test
find_package(GTest REQUIRED)

# Market domain tests
add_executable(market_tests domain/market_test.cpp)

target_link_libraries(market_tests PRIVATE
    GTest::gtest
    GTest::gtest_main
    domain
    project_warnings
)

target_include_directories(market_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Register the test
add_test(NAME MarketTests COMMAND market_tests)

# Matching engine domain tests
add_executable(matching_engine_tests domain/matching_engine_test.cpp)

target_link_libraries(matching_engine_tests PRIVATE
    GTest::gtest
    GTest::gtest_main
    domain
    project_warnings
)

target_include_directories(matching_engine_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Register the test
add_test(NAME MatchingEngineTests COMMAND matching_engine_tests)

# Top level domain tests
add_executable(top_level_tests domain/top_level_test.cpp)

target_link_libraries(top_level_tests PRIVATE
    GTest::gtest
    GTest::gtest_main
    src
    project_warnings
)

target_include_directories(top_level_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Register the test
add_test(NAME TopLevelTests COMMAND top_level_tests)

# Add custom target to run all tests
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS market_tests matching_engine_tests top_level_tests
    COMMENT "Running all tests..."
)
