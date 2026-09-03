#pragma once

#include "ast.hpp"
#include "flow.hpp"

#include <exprflow/graph/graph.hpp>

#include <any>
#include <functional>
#include <memory>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

struct Compiled {
  std::shared_ptr<TaskGraph> graph;
  std::vector<TaskGraph::NodeId> outputs;
};

template <typename Prev, typename F, std::size_t... I>
auto InvokeOnPrevImpl(const F& fn,
                      const std::vector<TaskGraph::NodeId>& ids,
                      std::vector<std::any>& results,
                      std::index_sequence<I...>)
    -> InvokeResultFromTupleT<F, OutputsT<Prev>> {
  using Tuple = OutputsT<Prev>;
  return std::invoke(fn, std::any_cast<const std::tuple_element_t<I, Tuple>&>(
                             results[ids[I]])...);
}

template <typename Prev, typename F>
auto InvokeOnPrev(const F& fn,
                  const std::vector<TaskGraph::NodeId>& ids,
                  std::vector<std::any>& results) {
  using Tuple = OutputsT<Prev>;
  return InvokeOnPrevImpl<Prev, F>(
      fn, ids, results, std::make_index_sequence<std::tuple_size_v<Tuple>>{});
}

template <typename T>
struct LeafTask {
  T* stored;

  auto operator()(std::vector<std::any>&) const -> std::any {
    return std::any{*stored};
  }
};

template <typename Prev, typename F>
struct InvokeTask {
  F* fn;
  const std::vector<TaskGraph::NodeId>* ids;

  auto operator()(std::vector<std::any>& results) const -> std::any {
    return std::any{InvokeOnPrev<Prev>(*fn, *ids, results)};
  }
};

template <typename Prev, typename Cond, typename ThenF, typename ElseF>
struct IfTask {
  Cond* cond;
  ThenF* then_fn;
  ElseF* else_fn;
  const std::vector<TaskGraph::NodeId>* ids;

  auto operator()(std::vector<std::any>& results) const -> std::any {
    if (InvokeOnPrev<Prev>(*cond, *ids, results)) {
      return std::any{InvokeOnPrev<Prev>(*then_fn, *ids, results)};
    }
    return std::any{InvokeOnPrev<Prev>(*else_fn, *ids, results)};
  }
};

template <typename Prev, typename F>
struct MapTask {
  F* fn;
  const std::vector<TaskGraph::NodeId>* ids;

  auto operator()(std::vector<std::any>& results) const -> std::any {
    using Range = std::tuple_element_t<0, OutputsT<Prev>>;
    using Elem = std::ranges::range_value_t<Range>;
    using Mapped = std::invoke_result_t<const F&, const Elem&>;
    const auto& range = std::any_cast<const Range&>(results[(*ids)[0]]);
    std::vector<Mapped> out;
    if constexpr (std::ranges::sized_range<Range>) {
      out.reserve(std::ranges::size(range));
    }
    for (const auto& elem : range) {
      out.push_back(std::invoke(*fn, elem));
    }
    return std::any{std::move(out)};
  }
};

template <typename Prev, typename Op, typename Seed>
struct FoldTask {
  Op* op;
  Seed* seed;
  const std::vector<TaskGraph::NodeId>* ids;

  auto operator()(std::vector<std::any>& results) const -> std::any {
    using Range = std::tuple_element_t<0, OutputsT<Prev>>;
    using Acc = std::decay_t<Seed>;
    const auto& range = std::any_cast<const Range&>(results[(*ids)[0]]);
    Acc acc = *seed;
    for (const auto& elem : range) {
      acc = std::invoke(*op, acc, elem);
    }
    return std::any{std::move(acc)};
  }
};

class Compiler {
  TaskGraph& graph_;

 public:
  explicit Compiler(TaskGraph& graph) : graph_(graph) {}

  template <typename Task>
  auto AddWired(const std::vector<TaskGraph::NodeId>* ids, Task task)
      -> TaskGraph::NodeId {
    const auto id = graph_.AddNode(std::move(task));
    for (const auto in : *ids) {
      graph_.AddEdge(in, id);
    }
    return id;
  }

  // Then / Every
  template <typename Prev, typename F>
  auto EmitInvoke(const std::vector<TaskGraph::NodeId>* ids, F fn)
      -> TaskGraph::NodeId {
    F* stored = graph_.Save(std::move(fn));
    return AddWired(ids, InvokeTask<Prev, F>{stored, ids});
  }

  template <typename T>
  auto EmitLeaf(const T& value) -> TaskGraph::NodeId {
    return graph_.AddNode(LeafTask<T>{graph_.Save(value)});
  }

  // ValueExpr
  template <typename... Ts>
  auto Emit(const ValueExpr<Ts...>& node) -> std::vector<TaskGraph::NodeId> {
    std::vector<TaskGraph::NodeId> ids;
    ids.reserve(sizeof...(Ts));
    std::apply(
        [&](const auto&... values) { (ids.push_back(EmitLeaf(values)), ...); },
        node.values);
    return ids;
  }

  // ThenExpr
  template <typename Prev, typename F>
  auto Emit(const ThenExpr<Prev, F>& node) -> std::vector<TaskGraph::NodeId> {
    auto* ids = graph_.Save(Emit(node.prev));
    return {EmitInvoke<Prev>(ids, node.fn)};
  }

  // EveryExpr
  template <typename Prev, typename... Fs>
  auto Emit(const EveryExpr<Prev, Fs...>& node)
      -> std::vector<TaskGraph::NodeId> {
    auto* ids = graph_.Save(Emit(node.prev));
    std::vector<TaskGraph::NodeId> outs;
    outs.reserve(sizeof...(Fs));
    std::apply(
        [&](const auto&... fns) {
          (outs.push_back(EmitInvoke<Prev>(ids, fns)), ...);
        },
        node.branches);
    return outs;
  }

  // IfExpr
  template <typename Prev, typename Cond, typename ThenF, typename ElseF>
  auto Emit(const IfExpr<Prev, Cond, ThenF, ElseF>& node)
      -> std::vector<TaskGraph::NodeId> {
    auto* ids = graph_.Save(Emit(node.prev));
    return {AddWired(ids, IfTask<Prev, Cond, ThenF, ElseF>{
                              graph_.Save(node.cond), graph_.Save(node.then_fn),
                              graph_.Save(node.else_fn), ids})};
  }

  // MapExpr
  template <typename Prev, typename F>
  auto Emit(const MapExpr<Prev, F>& node) -> std::vector<TaskGraph::NodeId> {
    auto* ids = graph_.Save(Emit(node.prev));
    return {AddWired(ids, MapTask<Prev, F>{graph_.Save(node.mapper), ids})};
  }

  // FoldExpr
  template <typename Prev, typename Op, typename Seed>
  auto Emit(const FoldExpr<Prev, Op, Seed>& node)
      -> std::vector<TaskGraph::NodeId> {
    auto* ids = graph_.Save(Emit(node.prev));
    return {
        AddWired(ids, FoldTask<Prev, Op, Seed>{graph_.Save(node.op),
                                               graph_.Save(node.seed), ids})};
  }
};

template <typename Node>
Compiled Compile(const Node& ast) {
  Compiled compiled;
  compiled.graph = std::make_shared<TaskGraph>();
  compiled.outputs = Compiler{*compiled.graph}.Emit(ast);
  compiled.graph->Seal();
  return compiled;
}

template <typename Node>
Compiled Compile(const Flow<Node>& flow) {
  return Compile(flow.Ast());
}
