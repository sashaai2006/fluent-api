#pragma once

#include "traits.hpp"

#include <tuple>
#include <utility>

namespace exprflow {

template <typename... Ts>
struct ValueExpr {
  std::tuple<Ts...> values;

  explicit ValueExpr(Ts... xs) : values(std::move(xs)...) {}
};

template <typename... Ts>
ValueExpr(Ts...) -> ValueExpr<Ts...>;

template <typename... Ts>
struct NodeOutputs<ValueExpr<Ts...>> {
  using type = std::tuple<Ts...>;
};

}  // namespace exprflow
