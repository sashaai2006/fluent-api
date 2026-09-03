#pragma once

#include "ast.hpp"

#include <type_traits>
#include <utility>

template <typename Node>
class Flow {
  Node node_;

 public:
  using Outputs = OutputsT<Node>;

  explicit Flow(Node node) : node_(std::move(node)) {}

  Flow(const Flow&) = delete;
  Flow& operator=(const Flow&) = delete;
  Flow(Flow&&) = default;
  Flow& operator=(Flow&&) = default;

  const Node& Ast() const { return node_; }

  template <typename NextNode>
  static auto From(NextNode node) -> Flow<NextNode>;

  template <typename F>
  auto Then(F&& f) &&;

  template <typename... Fs>
  auto Every(Fs&&... fs) &&;

  template <typename Pred, typename ThenF, typename ElseF>
  auto IfThenElse(Pred&& pred, ThenF&& then_fn, ElseF&& else_fn) &&;

  template <typename F>
  auto Map(F&& f) &&;

  template <typename Op, typename Seed>
  auto Fold(Op&& op, Seed&& seed) &&;
};

template <typename Node>
template <typename NextNode>
auto Flow<Node>::From(NextNode node) -> Flow<NextNode> {
  return Flow<NextNode>(std::move(node));
}

template <typename Node>
template <typename F>
auto Flow<Node>::Then(F&& f) && {
  return From(ThenExpr(std::move(node_), std::forward<F>(f)));
}

template <typename Node>
template <typename... Fs>
auto Flow<Node>::Every(Fs&&... fs) && {
  return From(EveryExpr(std::move(node_), std::forward<Fs>(fs)...));
}

template <typename Node>
template <typename Pred, typename ThenF, typename ElseF>
auto Flow<Node>::IfThenElse(Pred&& pred, ThenF&& then_fn, ElseF&& else_fn) && {
  return From(IfExpr(std::move(node_), std::forward<Pred>(pred),
                     std::forward<ThenF>(then_fn),
                     std::forward<ElseF>(else_fn)));
}

template <typename Node>
template <typename F>
auto Flow<Node>::Map(F&& f) && {
  return From(MapExpr(std::move(node_), std::forward<F>(f)));
}

template <typename Node>
template <typename Op, typename Seed>
auto Flow<Node>::Fold(Op&& op, Seed&& seed) && {
  return From(FoldExpr(std::move(node_), std::forward<Op>(op),
                       std::forward<Seed>(seed)));
}

template <typename Node>
Flow(Node) -> Flow<Node>;

template <typename... Ts>
auto Value(Ts&&... xs) {
  using Node = ValueExpr<std::decay_t<Ts>...>;
  return Flow<Node>(Node(std::forward<Ts>(xs)...));
}
