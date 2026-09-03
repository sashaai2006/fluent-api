#pragma once

#include <exprflow/flow/combinators/every.hpp>
#include <exprflow/flow/combinators/compile/runtime.hpp>

#include <vector>

namespace exprflow::detail {

template <typename C, typename Prev, typename... Fs>
auto EmitCombinator(C& c, const EveryExpr<Prev, Fs...>& node)
    -> std::vector<TaskGraph::NodeId> {
  auto* ids = c.Save(c.Emit(node.prev));
  std::vector<TaskGraph::NodeId> outs;
  outs.reserve(sizeof...(Fs));
  std::apply(
      [&](const auto&... fns) {
        (outs.push_back(c.template EmitInvoke<Prev>(ids, fns)), ...);
      },
      node.branches);
  return outs;
}

}  // namespace exprflow::detail
