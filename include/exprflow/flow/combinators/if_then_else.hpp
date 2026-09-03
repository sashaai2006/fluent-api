#pragma once

#include <exprflow/flow/combinators/traits.hpp>

#include <type_traits>
#include <utility>

namespace exprflow {

template <typename Prev, typename Cond, typename ThenF, typename ElseF>
struct IfExpr {
  Prev prev;
  Cond cond;
  ThenF then_fn;
  ElseF else_fn;

  IfExpr(Prev p, Cond c, ThenF t, ElseF e)
      : prev(std::move(p)),
        cond(std::move(c)),
        then_fn(std::move(t)),
        else_fn(std::move(e)) {}
};

template <typename Prev, typename Cond, typename ThenF, typename ElseF>
IfExpr(Prev, Cond, ThenF, ElseF) -> IfExpr<Prev, Cond, ThenF, ElseF>;

template <typename Prev, typename Cond, typename ThenF, typename ElseF>
struct NodeOutputs<IfExpr<Prev, Cond, ThenF, ElseF>> {
  using PrevOut = OutputsT<Prev>;
  using ThenOut = InvokeResultFromTupleT<ThenF, PrevOut>;
  using ElseOut = InvokeResultFromTupleT<ElseF, PrevOut>;
  using CondOut = InvokeResultFromTupleT<Cond, PrevOut>;

  static_assert(std::is_same_v<ThenOut, ElseOut>,
                "IfExpr: then and else must return the same type");
  static_assert(std::is_convertible_v<CondOut, bool>,
                "IfExpr: condition must be convertible to bool");

  using type = std::tuple<ThenOut>;
};

}  // namespace exprflow
