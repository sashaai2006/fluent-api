#pragma once

#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

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
