include(FetchContent)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.17.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

function(exprflow_add_unit_test target source)
  add_executable(${target} ${source})
  target_link_libraries(${target} PRIVATE exprflow::exprflow GTest::gtest_main)
  gtest_discover_tests(${target})
endfunction()

exprflow_add_unit_test(graph_unit test_graph.cpp)
exprflow_add_unit_test(executor_unit test_executor.cpp)
exprflow_add_unit_test(flow_unit test_flow.cpp)
exprflow_add_unit_test(compiler_unit test_compiler.cpp)
exprflow_add_unit_test(eval_unit test_eval.cpp)
exprflow_add_unit_test(block_queue_unit test_block_queue.cpp)
