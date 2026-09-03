#pragma once

#include <exprflow/flow/combinators/then.hpp>
#include <exprflow/flow/combinators/compile/runtime.hpp>

#include <vector>

namespace exprflow::detail {

template <typename C, typename Prev, typename F>
auto EmitCombinator(C& c, const ThenExpr<Prev, F>& node)
    -> std::vector<TaskGraph::NodeId> {
  auto* ids = c.Save(c.Emit(node.prev));
  return {c.template EmitInvoke<Prev>(ids, node.fn)};
}

}  // namespace exprflow::detail
