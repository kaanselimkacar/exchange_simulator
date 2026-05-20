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

# Add custom target to run all tests
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS market_tests
    COMMENT "Running all tests..."
)
