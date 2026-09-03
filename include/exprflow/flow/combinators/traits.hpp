#pragma once

#include <tuple>
#include <type_traits>
#include <utility>

namespace exprflow {

template <typename Node>
struct NodeOutputs;

template <typename Node>
using OutputsT = typename NodeOutputs<Node>::type;

template <typename F, typename Tuple>
struct InvokeResultFromTuple;

template <typename F, typename... Ts>
struct InvokeResultFromTuple<F, std::tuple<Ts...>> {
  using type = std::invoke_result_t<const std::decay_t<F>&, const Ts&...>;
};

template <typename F, typename Tuple>
using InvokeResultFromTupleT = typename InvokeResultFromTuple<F, Tuple>::type;

template <typename Op, typename Acc, typename Elem>
concept AssociativeOp =
    std::is_invocable_r_v<Acc, const Op&, const Acc&, const Elem&> &&
    std::is_invocable_r_v<Acc, const Op&, const Acc&, const Acc&>;

}  // namespace exprflow
