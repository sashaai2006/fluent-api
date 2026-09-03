#pragma once

#include <type_traits>

template <typename... Out>
class Flow {
 public:
  Flow(Flow&&) noexcept;
  Flow& operator=(Flow&&) noexcept;
  Flow(const Flow&) = delete;
  Flow& operator=(const Flow&) = delete;

  template <typename F>
  auto Then(F&& f)
      -> Flow<std::invoke_result_t<const std::decay_t<F>&, const Out&...>>;
};