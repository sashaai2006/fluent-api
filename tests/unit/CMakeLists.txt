include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.17.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(dag_unit test_graph.cpp)
target_link_libraries(dag_unit PRIVATE exprflow::exprflow GTest::gtest_main)

add_executable(executor_unit test_executor.cpp)
target_link_libraries(executor_unit PRIVATE exprflow::exprflow GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(dag_unit)
gtest_discover_tests(executor_unit)
