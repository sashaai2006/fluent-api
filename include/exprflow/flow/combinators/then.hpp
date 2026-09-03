#pragma once

#include "traits.hpp"

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

}  // namespace exprflow
