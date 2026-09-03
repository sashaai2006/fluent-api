#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "$0")" && pwd)"
jobs="$(nproc 2>/dev/null || sysctl -n hw.ncpu)"

usage() {
  echo "usage: $(basename "$0") build <fast|deep>" >&2
  exit 1
}

configure_build_test() {
  local dir="$1"
  shift
  cmake -S "$root" -B "$dir" "$@"
  cmake --build "$dir" --parallel "$jobs"
  ctest --test-dir "$dir" --output-on-failure
}

build_fast() {
  configure_build_test "$root/build/fast" -DCMAKE_BUILD_TYPE=Release
}

build_deep() {
  local asan="-fsanitize=address,undefined -fno-omit-frame-pointer"
  local tsan="-fsanitize=thread -fno-omit-frame-pointer"

  if [[ "$(uname -s)" == Linux ]]; then
    export ASAN_OPTIONS="${ASAN_OPTIONS:-halt_on_error=1:abort_on_error=1:detect_leaks=1}"
  else
    export ASAN_OPTIONS="${ASAN_OPTIONS:-halt_on_error=1:abort_on_error=1:detect_leaks=0}"
  fi
  export UBSAN_OPTIONS="${UBSAN_OPTIONS:-halt_on_error=1:print_stacktrace=1}"
  export TSAN_OPTIONS="${TSAN_OPTIONS:-halt_on_error=1}"

  configure_build_test "$root/build/deep-asan" \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_CXX_FLAGS="$asan" \
    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined" \
    -DCMAKE_SHARED_LINKER_FLAGS="-fsanitize=address,undefined"

  configure_build_test "$root/build/deep-tsan" \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_CXX_FLAGS="$tsan" \
    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=thread" \
    -DCMAKE_SHARED_LINKER_FLAGS="-fsanitize=thread"
}

[[ $# -eq 2 && $1 == build ]] || usage
case "$2" in
  fast) build_fast ;;
  deep) build_deep ;;
  *) usage ;;
esac
