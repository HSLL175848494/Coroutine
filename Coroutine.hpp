#ifndef HSLL_COROUTINE
#define HSLL_COROUTINE

#include <coroutine>
#include <optional>
#include <functional>
#include <stdexcept>

namespace HSLL
{
	enum START_FLAG
	{
		SUSPENDED = true,
		NOSUSPEND = false
	};

	template <START_FLAG>
	struct Suspend_Type;

	template <>
	struct Suspend_Type<SUSPENDED>
	{
		using value = std::suspend_always;
	};

	template <>
	struct Suspend_Type<NOSUSPEND>
	{
		using value = std::suspend_never;
	};

	template <START_FLAG FLAG, typename... Args>
	class Generator;

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

		Generator(const Generator&) = delete;
		Generator& operator=(const Generator&) = delete;

		void Destroy() noexcept {
			if (handle) {
				handle.destroy();
				handle = nullptr;
			}
		}

		bool HandleInvalid() const noexcept { return handle == nullptr; }

		bool hasDone() const {
			if (!handle) throw std::runtime_error("Accessing invalid coroutine handle");
			return handle.done();
		}

		bool Resume() {
			if (!handle) throw std::runtime_error("Resuming invalid coroutine handle");
			if (handle.done()) return false;
			handle.resume();
			return true;
		}

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

		Generator(const Generator&) = delete;
		Generator& operator=(const Generator&) = delete;

		void Destroy() noexcept {
			if (handle) {
				handle.destroy();
				handle = nullptr;
			}
		}

		bool HandleInvalid() const noexcept { return handle == nullptr; }
		bool hasDone() const {
			if (!handle) throw std::runtime_error("Accessing invalid coroutine handle");
			return handle.done();
		}

		bool Resume() {
			if (!handle) throw std::runtime_error("Resuming invalid coroutine handle");
			if (handle.done()) return false;
			handle.resume();
			return true;
		}
	};

	enum class AWAITABLE_STATE { Pending, Success, Exception };

	template<typename T, typename Scheduler>
	class Awaitable
	{
	private:
		AWAITABLE_STATE state;
		Scheduler& scheduler;
		std::exception_ptr exception;
		std::function<T()> callable;
		alignas(alignof(T)) char storage[sizeof(T)];

	public:

		template<typename Fn>
		Awaitable(Fn&& operation, Scheduler& scheduler)
			: callable(std::forward<Fn>(operation)), scheduler(scheduler), state(AWAITABLE_STATE::Pending) {}

		bool await_ready() const noexcept {
			return state != AWAITABLE_STATE::Pending;
		}

		void await_suspend(std::coroutine_handle<> handle) {
			scheduler([this, handle] {
				try {
					new (storage) T(callable());
					state = AWAITABLE_STATE::Success;
				}
				catch (...) {
					exception = std::current_exception();
					state = AWAITABLE_STATE::Exception;
				}

				handle.resume();
				});
		}

		auto await_resume() {
			if (state == AWAITABLE_STATE::Exception)
				std::rethrow_exception(exception);

			return std::move(*((T*)storage));
		}

		~Awaitable()
		{
			if (state == AWAITABLE_STATE::Success)
			{
				((T*)storage)->~T();
				state = AWAITABLE_STATE::Pending;
			}
		}
	};

	template<typename Scheduler>
	class Awaitable<void, Scheduler>
	{
	private:

		AWAITABLE_STATE state;
		Scheduler& scheduler;
		std::exception_ptr exception;
		std::function<void()> callable;

	public:

		template<typename Fn>
		Awaitable(Fn&& operation, Scheduler& scheduler)
			: callable(std::forward<Fn>(operation)), scheduler(scheduler), state(AWAITABLE_STATE::Pending) {}

		bool await_ready() const noexcept {
			return state != AWAITABLE_STATE::Pending;
		}

		void await_suspend(std::coroutine_handle<> handle) {
			scheduler([this, handle] {
				try {
					callable();
					state = AWAITABLE_STATE::Success;
				}
				catch (...) {
					exception = std::current_exception();
					state = AWAITABLE_STATE::Exception;
				}

				handle.resume();
				});
		}

		void await_resume() {
			if (state == AWAITABLE_STATE::Exception)
				std::rethrow_exception(exception);
		}

		~Awaitable()
		{
			state = AWAITABLE_STATE::Pending;
		}
	};
}

#endif