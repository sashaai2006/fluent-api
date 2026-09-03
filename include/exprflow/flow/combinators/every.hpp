#pragma once

#include "traits.hpp"

#include <tuple>
#include <utility>

namespace exprflow {

template <typename Prev, typename... Fs>
struct EveryExpr {
  Prev prev;
  std::tuple<Fs...> branches;

  EveryExpr(Prev p, Fs... fs)
      : prev(std::move(p)), branches(std::move(fs)...) {}
};

template <typename Prev, typename... Fs>
EveryExpr(Prev, Fs...) -> EveryExpr<Prev, Fs...>;

template <typename Prev, typename... Fs>
struct NodeOutputs<EveryExpr<Prev, Fs...>> {
  using type = std::tuple<InvokeResultFromTupleT<Fs, OutputsT<Prev>>...>;
};

}  // namespace exprflow
