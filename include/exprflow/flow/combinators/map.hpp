#pragma once

#include "traits.hpp"

#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace exprflow {

template <typename Prev, typename F>
struct MapExpr {
  Prev prev;
  F mapper;

  MapExpr(Prev p, F f) : prev(std::move(p)), mapper(std::move(f)) {}
};

template <typename Prev, typename F>
MapExpr(Prev, F) -> MapExpr<Prev, F>;

template <typename Prev, typename F>
struct NodeOutputs<MapExpr<Prev, F>> {
  using PrevOut = OutputsT<Prev>;
  static_assert(std::tuple_size_v<PrevOut> == 1,
                "MapExpr requires a single output");

  using Range = std::tuple_element_t<0, PrevOut>;
  static_assert(std::ranges::range<Range>, "MapExpr requires a range output");

  using Elem = std::ranges::range_value_t<Range>;
  using Mapped = std::invoke_result_t<const std::decay_t<F>&, const Elem&>;

  using type = std::tuple<std::vector<Mapped>>;
};

}  // namespace exprflow
