#pragma once

#include "traits.hpp"

#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>

namespace exprflow {

template <typename Prev, typename Op, typename Seed>
struct FoldExpr {
  Prev prev;
  Op op;
  Seed seed;

  FoldExpr(Prev p, Op o, Seed s)
      : prev(std::move(p)), op(std::move(o)), seed(std::move(s)) {}
};

template <typename Prev, typename Op, typename Seed>
FoldExpr(Prev, Op, Seed) -> FoldExpr<Prev, Op, Seed>;

template <typename Prev, typename Op, typename Seed>
struct NodeOutputs<FoldExpr<Prev, Op, Seed>> {
  using PrevOut = OutputsT<Prev>;
  static_assert(std::tuple_size_v<PrevOut> == 1,
                "FoldExpr requires a single output");

  using Range = std::tuple_element_t<0, PrevOut>;
  static_assert(std::ranges::range<Range>, "FoldExpr requires a range output");

  using Elem = std::ranges::range_value_t<Range>;
  using Acc = std::decay_t<Seed>;
  static_assert(AssociativeOp<std::decay_t<Op>, Acc, Elem>,
                "FoldExpr: op must be Acc(const Acc&, const Elem&) and "
                "Acc(const Acc&, const Acc&), and must be associative "
                "(op(a, op(b, c)) == op(op(a, b), c)) — parallel Fold "
                "combines partial accumulators of chunks");

  using type = std::tuple<Acc>;
};

}  // namespace exprflow
