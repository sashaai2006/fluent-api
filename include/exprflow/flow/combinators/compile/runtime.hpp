#pragma once

#include <exprflow/flow/combinators/traits.hpp>

#include <exprflow/graph/graph.hpp>

#include <algorithm>
#include <any>
#include <cstddef>
#include <functional>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace exprflow::detail {

struct ChunkPolicy {
  static std::size_t Count() {
    return std::max<std::size_t>(2, std::thread::hardware_concurrency());
  }

  static auto Bounds(std::size_t chunk, std::size_t chunks, std::size_t n)
      -> std::pair<std::size_t, std::size_t> {
    return {chunk * n / chunks, (chunk + 1) * n / chunks};
  }
};

template <typename T>
auto ReadResult(std::any& slot) -> const T& {
  if (auto* p = std::any_cast<T>(&slot)) {
    return *p;
  }
  return std::any_cast<std::reference_wrapper<const T>>(slot).get();
}

template <typename T>
auto TakeResult(std::any& slot) -> T {
  if (auto* p = std::any_cast<T>(&slot)) {
    return std::move(*p);
  }
  return std::any_cast<std::reference_wrapper<const T>>(slot).get();
}

template <typename Prev, typename F, std::size_t... I>
auto InvokeOnPrevImpl(const F& fn,
                      const std::vector<TaskGraph::NodeId>& ids,
                      std::vector<std::any>& results,
                      std::index_sequence<I...>)
    -> InvokeResultFromTupleT<F, OutputsT<Prev>> {
  using Tuple = OutputsT<Prev>;
  return std::invoke(
      fn, ReadResult<std::tuple_element_t<I, Tuple>>(results[ids[I]])...);
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
    return std::any{std::cref(*stored)};
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

}  // namespace exprflow::detail
