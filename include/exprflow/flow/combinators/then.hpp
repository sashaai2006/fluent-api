#pragma once

#include <exprflow/flow/combinators/traits.hpp>

#include <type_traits>
#include <utility>

namespace exprflow {

template <typename Prev, typename F>
struct ThenExpr {
  Prev prev;
  F fn;

  ThenExpr(Prev p, F f) : prev(std::move(p)), fn(std::move(f)) {}
};

template <typename Prev, typename F>
ThenExpr(Prev, F) -> ThenExpr<Prev, F>;

template <typename Prev, typename F>
struct NodeOutputs<ThenExpr<Prev, F>> {
  using type = std::tuple<InvokeResultFromTupleT<F, OutputsT<Prev>>>;
};

template <typename T>
struct IsThenExpr final : std::false_type {};

template <typename Prev, typename F>
struct IsThenExpr<ThenExpr<Prev, F>> final : std::true_type {};

template <typename T>
inline constexpr bool is_then_expr_v = IsThenExpr<T>::value;

}  // namespace exprflow
