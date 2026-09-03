#pragma once

#include <exprflow/executor.hpp>
#include <exprflow/flow/compiler.hpp>
#include <exprflow/flow/flow.hpp>

#include <any>
#include <tuple>
#include <utility>

template <typename Node>
using EvalResultT = std::tuple_element_t<0, typename Flow<Node>::Outputs>;

template <typename Node>
auto Eval(Flow<Node>&& flow, Executor& executor) -> EvalResultT<Node> {
  static_assert(std::tuple_size_v<typename Flow<Node>::Outputs> == 1,
                "Eval: expression must have a single output");
  auto compiled = Compile(flow);
  auto results = executor.Submit(compiled.graph).get();
  return std::any_cast<EvalResultT<Node>>(
      std::move(results[compiled.outputs[0]]));
}

template <typename Node>
auto Eval(Flow<Node>&& flow) -> EvalResultT<Node> {
  Executor executor;
  return Eval(std::move(flow), executor);
}
