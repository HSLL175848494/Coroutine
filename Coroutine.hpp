#ifndef HSLL_COROUTINE
#define HSLL_COROUTINE

#include <coroutine>
#include <optional>
#include <stdexcept>

namespace HSLL
{
    enum START_FLAG
    {
        START_FLAG_SUSPENDED = true,
        START_FLAG_NOSUSPEND = false
    };

    template <START_FLAG>
    struct Suspend_Type;

    template <>
    struct Suspend_Type<START_FLAG_SUSPENDED>
    {
        using value = std::suspend_always;
    };

    template <>
    struct Suspend_Type<START_FLAG_NOSUSPEND>
    {
        using value = std::suspend_never;
    };

    template <START_FLAG FLAG, typename... Args>
    class Generator;

    // Generator with value type
    template <START_FLAG FLAG, class T>
    class Generator<FLAG, T>
    {
    public:
        struct promise_type
        {
            std::optional<T> optional;

            auto get_return_object() noexcept {
                return Generator{ std::coroutine_handle<promise_type>::from_promise(*this) };
            }

            Suspend_Type<FLAG>::value initial_suspend() noexcept { return {}; }
            std::suspend_always final_suspend() noexcept { return {}; }

            std::suspend_always yield_value(T value) noexcept {
                optional = std::move(value);
                return {};
            }

            void return_value(T value) { optional = std::move(value); }
            void unhandled_exception() { std::terminate(); }
        };

    private:
        std::coroutine_handle<promise_type> handle = nullptr;

    public:
        Generator() = default;
        explicit Generator(std::coroutine_handle<promise_type> h) noexcept : handle(h) {}

        ~Generator() { Destroy(); }

        // Move operations
        Generator(Generator&& other) noexcept : handle(other.handle) {
            other.handle = nullptr;
        }

        Generator& operator=(Generator&& other) noexcept {
            if (this != &other) {
                Destroy();
                handle = other.handle;
                other.handle = nullptr;
            }
            return *this;
        }

        // Non-copyable
        Generator(const Generator&) = delete;
        Generator& operator=(const Generator&) = delete;

        // Resource management
        void Destroy() noexcept {
            if (handle) {
                handle.destroy();
                handle = nullptr;
            }
        }

        // State checks
        bool HandleInvalid() const noexcept { return handle == nullptr; }
        bool hasDone() const {
            if (!handle) throw std::runtime_error("Accessing invalid coroutine handle");
            return handle.done();
        }

        // Control flow
        bool Resume() {
            if (!handle) throw std::runtime_error("Resuming invalid coroutine handle");
            if (handle.done()) return false;
            handle.resume();
            return true;
        }

        // Value access
        std::optional<T> next() {
            if (!handle) return std::nullopt;
            if (!handle.done()) {
                handle.resume();
                return handle.promise().optional;
            }
            return std::nullopt;
        }

        std::optional<T> Value() const noexcept {
            return handle ? handle.promise().optional : std::nullopt;
        }
    };

    // Void specialization
    template <START_FLAG FLAG>
    class Generator<FLAG>
    {
    public:
        struct promise_type
        {
            Generator get_return_object() noexcept {
                return Generator{ std::coroutine_handle<promise_type>::from_promise(*this) };
            }

            Suspend_Type<FLAG>::value initial_suspend() noexcept { return {}; }
            std::suspend_always final_suspend() noexcept { return {}; }
            void return_void() noexcept {}
            void unhandled_exception() { std::terminate(); }
        };

    private:
        std::coroutine_handle<promise_type> handle = nullptr;

    public:
        Generator() = default;
        explicit Generator(std::coroutine_handle<promise_type> h) noexcept : handle(h) {}

        ~Generator() { Destroy(); }

        // Move operations
        Generator(Generator&& other) noexcept : handle(other.handle) {
            other.handle = nullptr;
        }

        Generator& operator=(Generator&& other) noexcept {
            if (this != &other) {
                Destroy();
                handle = other.handle;
                other.handle = nullptr;
            }
            return *this;
        }

        // Non-copyable
        Generator(const Generator&) = delete;
        Generator& operator=(const Generator&) = delete;

        // Resource management
        void Destroy() noexcept {
            if (handle) {
                handle.destroy();
                handle = nullptr;
            }
        }

        // State checks
        bool HandleInvalid() const noexcept { return handle == nullptr; }
        bool hasDone() const {
            if (!handle) throw std::runtime_error("Accessing invalid coroutine handle");
            return handle.done();
        }

        // Control flow
        bool Resume() {
            if (!handle) throw std::runtime_error("Resuming invalid coroutine handle");
            if (handle.done()) return false;
            handle.resume();
            return true;
        }
    };
}

#endif