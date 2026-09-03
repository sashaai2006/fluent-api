# exprflow

C++23 библиотека для построения и исполнения ациклических графов задач (DAG) через fluent API выражений.

## Возможности

- `TaskGraph` — компактное представление DAG с топологической сортировкой и проверкой на циклы.
- `Executor` — пул потоков с поддержкой одновременного запуска нескольких графов.
- `Flow` / `Eval` — типобезопасное (на этапе компиляции) построение вычислительных цепочек.
- Операторы: `Value`, `Then`, `Every`, `IfThenElse`, `Map`, `Fold`.
- `BlockQueue` — lock-free блокирующая очередь MPMC для планировщика.
- C++23, зависимости только для тестов (GoogleTest).

## Требования

- CMake >= 3.25
- Компилятор с поддержкой C++23 (GCC 13+, Clang 16+, Apple Clang 15+)
- Docker (опционально)

## Сборка и тесты

### Быстрая сборка

```bash
bash harness.sh build fast
```

### Глубокая сборка с санитайзерами

```bash
bash harness.sh build deep
```

### Вручную

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

### Docker

```bash
docker build -t exprflow .
docker run --rm exprflow
```

## Пример использования

```cpp
#include <exprflow/eval.hpp>
#include <iostream>
#include <vector>

int main() {
    auto result = Eval(
        Value(std::vector<int>{1, 2, 3, 4, 5})
            .Map([](int x) { return x * x; })
            .Fold([](int acc, int x) { return acc + x; }, 0)
    );
    std::cout << result << '\n';  // 55
    return 0;
}
```

С переиспользуемым экзекьютором:

```cpp
#include <exprflow/eval.hpp>

Executor executor(4);

auto result = Eval(
    Value(2, 3).Then([](int a, int b) { return a + b; }),
    executor
);
// result == 5
```

## Структура проекта

```
include/exprflow/        публичные заголовки
  graph/                   TaskGraph, InplaceFunction
  sync/                    BlockQueue
  flow/                    AST, Compiler, Flow API
  eval.hpp                 удобная обертка Eval
  executor.hpp             пул потоков
src/exprflow/            реализация
tests/unit/              unit-тесты на GoogleTest
```

## Статус

MVP готов. Библиотека покрыта unit-тестами и собирается с AddressSanitizer / ThreadSanitizer.
