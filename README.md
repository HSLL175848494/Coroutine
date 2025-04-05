Here's a README.md file for your coroutine generator library:

```markdown
# HSLL Coroutine Generator Library

A C++20 header-only library providing generator coroutines with configurable suspension behavior.

## Requirements

- C++20 compiler with coroutine support
- Standard Library with `<coroutine>` header

## Usage

### Basic Generator with Return Value

```cpp
#include "coroutine.hpp"

HSLL::Generator<HSLL::START_FLAG_SUSPENDED, int> generate_numbers(int max) {
    for (int i = 0; i < max; ++i) {
        co_yield i;
    }
    co_return -1;  // Final value
}

int main() {
    auto gen = generate_numbers(5);
    while (auto val = gen.next()) {
        std::cout << *val << "\n";
    }
}
```

### Void Generator (No Return Value)

```cpp
HSLL::Generator<HSLL::START_FLAG_NOSUSPEND> task() {
    std::cout << "Task started\n";
    co_await std::suspend_always{};
    std::cout << "Task resumed\n";
}

int main() {
    auto t = task();  // Starts immediately due to START_FLAG_NOSUSPEND
    t.Resume();
}
```

## API Reference

### Generator Class

#### Template Parameters
- `FLAG`: `START_FLAG_SUSPENDED` or `START_FLAG_NOSUSPEND`
- `T`: (Optional) Type of values to generate

#### Methods
- `next()`: Resume execution and get next value (returns `std::optional<T>`)
- `Resume()`: Resume execution without returning a value (for void generators)
- `Value()`: Get current value without resuming
- `hasDone()`: Check if coroutine has completed
- `Destroy()`: Explicitly destroy the coroutine
- `HandleInvalid()`: Check if handle is valid

### START_FLAG Enum
- `START_FLAG_SUSPENDED`: Coroutine starts suspended
- `START_FLAG_NOSUSPEND`: Coroutine starts immediately

## Best Practices

1. Always check `hasDone()` before resuming
2. Prefer `next()` for value-generating coroutines
3. Use `Destroy()` explicitly if you need to clean up before the generator goes out of scope
4. The generator is move-only - use `std::move` when transferring ownership
3. Complete API documentation
4. Best practices
5. Requirements and license information

You may want to customize the license section based on your actual licensing choice. The examples demonstrate both the value-generating and void generator use cases, showing the flexibility of your implementation.
