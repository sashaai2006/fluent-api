# Fluent-api

`Библиотека для выражения параллельных вычислений как fluent-цепочки`

## Идея

```cpp
#include <exprflow/eval.hpp>

using namespace exprflow;

auto result = Eval(
    Value(2, 3)
        .Every([](int a, int b) { return a + b; },
               [](int a, int b) { return a * b; })
        .Then([](int sum, int prod) { return sum + prod; }));
// result == 11
```

## Поддерживаемые комбинаторы

Список будет расширяться.

| | Комбинатор | Пример | |
| --- | --- | --- | --- |
| 1 | `Value` | `Value(12)` / `Value(1, 2, 3)` | `·` или `(·, …, ·)` |
| 2 | `Then` | `.Then([](auto x) { return …; })` | `f ∘ F` |
| 3 | `Every` | `.Every(f1, f2, …).Then([](auto x1, auto x2, …) { return …; })` | `⟨·, …, ·⟩` |
| 4 | `IfThenElse` | `.IfThenElse(pred, then_f, else_f)` | `if(·) then · else ·` |
| 5 | `Map` | `Value(std::vector{a1, …, an}).Map([](auto x) { return …; })` | `map` |
| 6 | `Fold` | `Value(std::vector{a1, …, an}).Fold([](auto acc, auto x) { return …; }, seed)` | `fold` |
