#pragma once

#include "../value.hpp"
#include "runtime.hpp"

#include <vector>

namespace exprflow::detail {

template <typename C, typename... Ts>
auto EmitCombinator(C& c, const ValueExpr<Ts...>& node)
    -> std::vector<TaskGraph::NodeId> {
  std::vector<TaskGraph::NodeId> ids;
  ids.reserve(sizeof...(Ts));
  std::apply(
      [&](const auto&... values) { (ids.push_back(c.EmitLeaf(values)), ...); },
      node.values);
  return ids;
}

}  // namespace exprflow::detail
