# HSLL 协程生成器库

一个基于 C++20 的仅头文件库，提供可配置挂起行为的生成器协程

## 要求

- 支持协程的 C++20 编译器
- 包含 `<coroutine>` 头的标准库

## 使用方法

### 值类型生成器
```cpp
using namespace HSLL;

Generator<START_FLAG_SUSPENDED, int> generate_numbers(int max) {
    for (int i = 0; i < max; ++i) {
        co_yield i;
    }
    co_return -1;  // 最终值
}

int main() {
    auto gen = generate_numbers(5);
    while (auto val = gen.next()) {
        std::cout << *val << "\n";
    }
}
```

### 无返回值生成器

```cpp
using namespace HSLL;

Generator<START_FLAG_NOSUSPEND> task() {
    std::cout << "任务开始\n";
    co_await std::suspend_always{};
    std::cout << "任务恢复\n";
}

int main() {
    auto t = task();  // 由于 START_FLAG_NOSUSPEND 立即开始执行
    t.Resume();
}
```

## API 参考

### Generator 类

#### 模板参数
- `FLAG`: 启动标志 (`START_FLAG_SUSPENDED` 或 `START_FLAG_NOSUSPEND`)
- `T`: (可选) 生成值的类型

#### 方法
- `next()`: 恢复执行并获取下一个值 (返回 `std::optional<T>`)
- `Resume()`: 恢复执行但不返回值 (用于 void 生成器)
- `Value()`: 获取当前值而不恢复执行
- `hasDone()`: 检查协程是否已完成
- `Destroy()`: 显式销毁协程
- `HandleInvalid()`: 检查句柄是否有效

### START_FLAG 枚举
- `START_FLAG_SUSPENDED`: 协程以挂起状态开始
- `START_FLAG_NOSUSPEND`: 协程立即开始执行

## 最佳实践

1. 在恢复前始终检查 `hasDone()`
2. 对值生成协程优先使用 `next()` 方法
3. 需要在生成器超出作用域前清理时显式调用 `Destroy()`
4. 生成器仅可移动 - 转移所有权时使用 `std::move`
5. 避免在已完成的生成器上调用恢复方法
6. 对 void 生成器使用专门的 `Resume()` 方法

## 实现细节

### 生命周期管理
- 生成器对象析构时会自动销毁关联的协程
- 协程帧使用 `std::coroutine_handle` 进行管理
- 支持 RAII 风格的资源管理

### 异常处理
- 协程内未捕获的异常会传播到调用方
- 建议在协程内部进行异常处理
