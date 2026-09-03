#pragma once

#include "../fold.hpp"
#include "runtime.hpp"

#include <any>
#include <cstddef>
#include <optional>
#include <ranges>
#include <type_traits>
#include <vector>

namespace exprflow::detail {

template <typename Prev, typename Op, typename Seed>
struct FoldTask {
  Op* op;
  Seed* seed;
  const std::vector<TaskGraph::NodeId>* ids;

  auto operator()(std::vector<std::any>& results) const -> std::any {
    using Range = std::tuple_element_t<0, OutputsT<Prev>>;
    using Acc = std::decay_t<Seed>;
    const auto& range = ReadResult<Range>(results[(*ids)[0]]);
    Acc acc = *seed;
    for (const auto& elem : range) {
      acc = std::invoke(*op, acc, elem);
    }
    return std::any{std::move(acc)};
  }
};

template <typename Prev, typename Op, typename Seed>
struct FoldChunkTask {
  Op* op;
  const std::vector<TaskGraph::NodeId>* ids;
  std::size_t chunk;
  std::size_t chunks;

  auto operator()(std::vector<std::any>& results) const -> std::any {
    using Range = std::tuple_element_t<0, OutputsT<Prev>>;
    using Acc = std::decay_t<Seed>;
    const auto& range = ReadResult<Range>(results[(*ids)[0]]);
    const auto [begin, end] =
        ChunkPolicy::Bounds(chunk, chunks, std::ranges::size(range));
    if (begin == end) {
      return std::any{std::optional<Acc>{}};
    }
    auto it = std::ranges::next(std::ranges::begin(range),
                                static_cast<std::ptrdiff_t>(begin));
    Acc acc{*it};
    ++it;
    for (std::size_t i = begin + 1; i < end; ++i, ++it) {
      acc = std::invoke(*op, acc, *it);
    }
    return std::any{std::optional<Acc>{std::move(acc)}};
  }
};

template <typename Prev, typename Op, typename Seed>
struct FoldJoinTask {
  Op* op;
  Seed* seed;
  const std::vector<TaskGraph::NodeId>* ids;

  auto operator()(std::vector<std::any>& results) const -> std::any {
    using Acc = std::decay_t<Seed>;
    Acc acc = *seed;
    for (const auto id : *ids) {
      auto& partial = std::any_cast<std::optional<Acc>&>(results[id]);
      if (partial.has_value()) {
        acc = std::invoke(*op, acc, *partial);
      }
    }
    return std::any{std::move(acc)};
  }
};

template <typename C, typename Prev, typename Op, typename Seed>
auto EmitCombinator(C& c, const FoldExpr<Prev, Op, Seed>& node)
    -> std::vector<TaskGraph::NodeId> {
  auto* ids = c.Save(c.Emit(node.prev));
  Op* op = c.Save(node.op);
  Seed* seed = c.Save(node.seed);
  using Range = std::tuple_element_t<0, OutputsT<Prev>>;
  using Elem = std::ranges::range_value_t<Range>;
  using Acc = std::decay_t<Seed>;
  if constexpr (std::ranges::sized_range<Range> &&
                std::constructible_from<Acc, const Elem&>) {
    const std::size_t chunks = ChunkPolicy::Count();
    std::vector<TaskGraph::NodeId> chunk_ids;
    chunk_ids.reserve(chunks);
    for (std::size_t i = 0; i < chunks; ++i) {
      chunk_ids.push_back(
          c.AddWired(ids, FoldChunkTask<Prev, Op, Seed>{op, ids, i, chunks}));
    }
    auto* join_ids = c.Save(std::move(chunk_ids));
    return {
        c.AddWired(join_ids, FoldJoinTask<Prev, Op, Seed>{op, seed, join_ids})};
  } else {
    return {c.AddWired(ids, FoldTask<Prev, Op, Seed>{op, seed, ids})};
  }
}

}  // namespace exprflow::detail
