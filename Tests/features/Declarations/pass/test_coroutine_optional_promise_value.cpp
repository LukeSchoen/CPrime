// EXPECT_COMPILE_ARGS: -fcoroutines
// EXPECT_COMPILE_ONLY: 1

// GCC row pr100127.C.  A promise can store its co_return value in an optional
// member, and initial_suspend may return a local awaiter class that holds a
// reference to that member.

#include <coroutine>
#include <optional>

struct future
{
  using value_type = int;
  struct promise_type;
  using handle_type = std::coroutine_handle<promise_type>;

  handle_type coroutine;

  future(handle_type h) : coroutine(h) {}
  ~future()
  {
    if (coroutine)
      coroutine.destroy();
  }

  struct promise_type
  {
    std::optional<value_type> value = std::nullopt;

    future get_return_object()
    {
      return future{handle_type::from_promise(*this)};
    }

    void return_value(value_type v)
    {
      value = static_cast<value_type&&>(v);
    }

    auto initial_suspend() noexcept
    {
      class awaiter
      {
        std::optional<value_type>& value;
      public:
        explicit awaiter(std::optional<value_type>& v) noexcept : value(v) {}
        bool await_ready() noexcept { return value.has_value(); }
        void await_suspend(handle_type) noexcept {}
        value_type& await_resume() noexcept { return *value; }
      };

      return awaiter{value};
    }

    std::suspend_always final_suspend() noexcept { return {}; }
    void unhandled_exception() {}
  };
};

future create()
{
  co_return 2021;
}
