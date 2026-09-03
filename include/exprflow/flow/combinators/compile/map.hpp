#pragma once

#include <exprflow/flow/combinators/map.hpp>
#include <exprflow/flow/combinators/compile/runtime.hpp>

#include <any>
#include <cstddef>
#include <iterator>
#include <ranges>
#include <vector>

namespace exprflow::detail {

template <typename Prev, typename F>
struct MapTask {
  F* fn;
  const std::vector<TaskGraph::NodeId>* ids;

  auto operator()(std::vector<std::any>& results) const -> std::any {
    using Range = std::tuple_element_t<0, OutputsT<Prev>>;
    using Elem = std::ranges::range_value_t<Range>;
    using Mapped = std::invoke_result_t<const F&, const Elem&>;
    const auto& range = ReadResult<Range>(results[(*ids)[0]]);
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

template <typename Prev, typename F>
struct MapChunkTask {
  F* fn;
  const std::vector<TaskGraph::NodeId>* ids;
  std::size_t chunk;
  std::size_t chunks;

  auto operator()(std::vector<std::any>& results) const -> std::any {
    using Range = std::tuple_element_t<0, OutputsT<Prev>>;
    using Elem = std::ranges::range_value_t<Range>;
    using Mapped = std::invoke_result_t<const F&, const Elem&>;
    const auto& range = ReadResult<Range>(results[(*ids)[0]]);
    const auto [begin, end] =
        ChunkPolicy::Bounds(chunk, chunks, std::ranges::size(range));
    std::vector<Mapped> out;
    out.reserve(end - begin);
    auto it = std::ranges::next(std::ranges::begin(range),
                                static_cast<std::ptrdiff_t>(begin));
    for (std::size_t i = begin; i < end; ++i, ++it) {
      out.push_back(std::invoke(*fn, *it));
    }
    return std::any{std::move(out)};
  }
};

template <typename Prev, typename F>
struct MapJoinTask {
  const std::vector<TaskGraph::NodeId>* ids;

  auto operator()(std::vector<std::any>& results) const -> std::any {
    using Range = std::tuple_element_t<0, OutputsT<Prev>>;
    using Elem = std::ranges::range_value_t<Range>;
    using Mapped = std::invoke_result_t<const F&, const Elem&>;
    std::size_t total = 0;
    for (const auto id : *ids) {
      total += std::any_cast<const std::vector<Mapped>&>(results[id]).size();
    }
    std::vector<Mapped> out;
    out.reserve(total);
    for (const auto id : *ids) {
      auto& part = std::any_cast<std::vector<Mapped>&>(results[id]);
      std::move(part.begin(), part.end(), std::back_inserter(out));
    }
    return std::any{std::move(out)};
  }
};

template <typename C, typename Prev, typename F>
auto EmitCombinator(C& c, const MapExpr<Prev, F>& node)
    -> std::vector<TaskGraph::NodeId> {
  auto* ids = c.Save(c.Emit(node.prev));
  F* fn = c.Save(node.mapper);
  using Range = std::tuple_element_t<0, OutputsT<Prev>>;
  if constexpr (std::ranges::sized_range<Range>) {
    const std::size_t chunks = ChunkPolicy::Count();
    std::vector<TaskGraph::NodeId> chunk_ids;
    chunk_ids.reserve(chunks);
    for (std::size_t i = 0; i < chunks; ++i) {
      chunk_ids.push_back(
          c.AddWired(ids, MapChunkTask<Prev, F>{fn, ids, i, chunks}));
    }
    auto* join_ids = c.Save(std::move(chunk_ids));
    return {c.AddWired(join_ids, MapJoinTask<Prev, F>{join_ids})};
  } else {
    return {c.AddWired(ids, MapTask<Prev, F>{fn, ids})};
  }
}

}  // namespace exprflow::detail
