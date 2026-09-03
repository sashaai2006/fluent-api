#pragma once

#include <exprflow/flow/combinators/if_then_else.hpp>
#include <exprflow/flow/combinators/compile/runtime.hpp>

#include <any>
#include <vector>

namespace exprflow::detail {

template <typename Prev, typename Cond, typename ThenF, typename ElseF>
struct IfTask {
  Cond* cond;
  ThenF* then_fn;
  ElseF* else_fn;
  const std::vector<TaskGraph::NodeId>* ids;

  auto operator()(std::vector<std::any>& results) const -> std::any {
    if (InvokeOnPrev<Prev>(*cond, *ids, results)) {
      return std::any{InvokeOnPrev<Prev>(*then_fn, *ids, results)};
    }
    return std::any{InvokeOnPrev<Prev>(*else_fn, *ids, results)};
  }
};

template <typename C,
          typename Prev,
          typename Cond,
          typename ThenF,
          typename ElseF>
auto EmitCombinator(C& c, const IfExpr<Prev, Cond, ThenF, ElseF>& node)
    -> std::vector<TaskGraph::NodeId> {
  auto* ids = c.Save(c.Emit(node.prev));
  return {c.AddWired(ids, IfTask<Prev, Cond, ThenF, ElseF>{
                              c.Save(node.cond), c.Save(node.then_fn),
                              c.Save(node.else_fn), ids})};
}

}  // namespace exprflow::detail
