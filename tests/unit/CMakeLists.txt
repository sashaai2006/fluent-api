find_package(GTest REQUIRED)

add_executable(dag_unit test_graph.cpp)
target_link_libraries(dag_unit PRIVATE dag::dag GTest::gtest_main)

add_executable(executor_unit test_executor.cpp)
target_link_libraries(executor_unit PRIVATE executor::executor GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(dag_unit)
gtest_discover_tests(executor_unit)
