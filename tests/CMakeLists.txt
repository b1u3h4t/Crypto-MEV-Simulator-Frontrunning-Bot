# Test configuration
enable_testing()

# Create test executable
add_executable(mev_tests
    unit_tests.cpp
    strategy_tests.cpp
    performance_tests.cpp
)

# Link libraries
target_link_libraries(mev_tests
    PRIVATE
    gtest
    gtest_main
    gmock
    gmock_main
)

# Include directories
target_include_directories(mev_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
)

# Add tests
add_test(NAME UnitTests COMMAND mev_tests --gtest_filter=UnitTest*)
add_test(NAME StrategyTests COMMAND mev_tests --gtest_filter=StrategyTest*)
add_test(NAME PerformanceTests COMMAND mev_tests --gtest_filter=PerformanceTest*)

# Set test properties
set_tests_properties(UnitTests PROPERTIES TIMEOUT 30)
set_tests_properties(StrategyTests PROPERTIES TIMEOUT 60)
set_tests_properties(PerformanceTests PROPERTIES TIMEOUT 120)
