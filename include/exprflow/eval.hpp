#pragma once

#include <exprflow/executor.hpp>
#include <exprflow/flow/compiler.hpp>
#include <exprflow/flow/flow.hpp>

#include <any>
#include <functional>
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
  std::any& slot = results[compiled.outputs[0]];
  using T = EvalResultT<Node>;
  if (auto* p = std::any_cast<T>(&slot)) {
    return std::move(*p);
  }
  return std::any_cast<std::reference_wrapper<const T>>(slot).get();
}

template <typename Node>
auto Eval(Flow<Node>&& flow) -> EvalResultT<Node> {
  Executor executor;
  return Eval(std::move(flow), executor);
}
