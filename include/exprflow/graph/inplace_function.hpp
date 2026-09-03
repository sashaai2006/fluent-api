#pragma once

#include <concepts>
#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>

template <typename Signature, std::size_t Capacity = 48>
class InplaceFunction;

template <typename R, typename... Args, std::size_t Capacity>
class InplaceFunction<R(Args...), Capacity> {
 private:
  static constexpr std::size_t kAlignment = alignof(std::max_align_t);

  struct Vtable {
    R (*Invoke)(const void*, Args...);
    void (*Destroy)(void*) noexcept;
    void (*Move)(void*, void*) noexcept;
  };

  alignas(kAlignment) std::byte storage_[Capacity];
  const Vtable* vt_{nullptr};

 public:
  InplaceFunction() noexcept = default;

  ~InplaceFunction();

  InplaceFunction(InplaceFunction&& other) noexcept;

  InplaceFunction(const InplaceFunction&) = delete;
  InplaceFunction& operator=(const InplaceFunction&) = delete;
  InplaceFunction& operator=(InplaceFunction&& other) noexcept;

  template <typename Fn>
  InplaceFunction(Fn&& fn)
    requires(!std::same_as<std::remove_cvref_t<Fn>,
                           InplaceFunction<R(Args...), Capacity>>);

  explicit operator bool() const noexcept;

  R operator()(Args... args) const;

 private:
  void MoveFrom(InplaceFunction& other) noexcept;
  void Reset() noexcept;
};

template <typename R, typename... Args, std::size_t Capacity>
InplaceFunction<R(Args...), Capacity>::~InplaceFunction() {
  Reset();
}

template <typename R, typename... Args, std::size_t Capacity>
InplaceFunction<R(Args...), Capacity>::InplaceFunction(InplaceFunction&& other) noexcept {
  MoveFrom(other);
}

template <typename R, typename... Args, std::size_t Capacity>
template <typename Fn>
InplaceFunction<R(Args...), Capacity>::InplaceFunction(Fn&& fn)
  requires(
      !std::same_as<std::remove_cvref_t<Fn>, InplaceFunction<R(Args...), Capacity>>)
{
  using clean_fn = std::remove_cvref_t<Fn>;
  static_assert(std::is_move_constructible_v<clean_fn>,
                "Fn must be move constructible");
  static_assert(std::is_destructible_v<clean_fn>, "Fn must be destructible");
  static_assert(std::is_invocable_r_v<R, const clean_fn&, Args...>,
                "Fn must be const-invocable as R(Args...)");
  static_assert(sizeof(clean_fn) <= Capacity,
                "Fn does not fit in inline storage");
  static_assert(alignof(clean_fn) <= kAlignment,
                "Fn alignment exceeds inline storage alignment");

  new (storage_) clean_fn(std::forward<Fn>(fn));

  static const Vtable vt = {
      .Invoke = [](const void* ptr, Args... args) -> R {
        return std::invoke(*static_cast<const clean_fn*>(ptr),
                           std::forward<Args>(args)...);
      },
      .Destroy =
          [](void* ptr) noexcept { static_cast<clean_fn*>(ptr)->~clean_fn(); },
      .Move =
          [](void* dst, void* src) noexcept {
            new (dst) clean_fn(std::move(*static_cast<clean_fn*>(src)));
            static_cast<clean_fn*>(src)->~clean_fn();
          },
  };
  vt_ = &vt;
}

template <typename R, typename... Args, std::size_t Capacity>
InplaceFunction<R(Args...), Capacity>::operator bool() const noexcept {
  return vt_ != nullptr;
}

template <typename R, typename... Args, std::size_t Capacity>
R InplaceFunction<R(Args...), Capacity>::operator()(Args... args) const {
  if (!vt_) {
    throw std::bad_function_call{};
  }
  return vt_->Invoke(storage_, std::forward<Args>(args)...);
}

template <typename R, typename... Args, std::size_t Capacity>
InplaceFunction<R(Args...), Capacity>& InplaceFunction<R(Args...), Capacity>::operator=(
    InplaceFunction&& other) noexcept {
  if (this != &other) {
    Reset();
    MoveFrom(other);
  }
  return *this;
}

template <typename R, typename... Args, std::size_t Capacity>
void InplaceFunction<R(Args...), Capacity>::MoveFrom(InplaceFunction& other) noexcept {
  if (other.vt_) {
    other.vt_->Move(storage_, other.storage_);
    vt_ = other.vt_;
    other.vt_ = nullptr;
  }
}

template <typename R, typename... Args, std::size_t Capacity>
void InplaceFunction<R(Args...), Capacity>::Reset() noexcept {
  if (vt_) {
    vt_->Destroy(storage_);
    vt_ = nullptr;
  }
}
