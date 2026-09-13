// EXPECT_COMPILE_ARGS: -fcoroutines
// EXPECT_COMPILE_ONLY: 1

// A `co_await` operand built from a live local has to be complete before the
// suspension point: an object whose destructor is not noexcept, an argument
// reached through a smart-pointer dereference, and an argument produced by a
// virtual member call.  These are the shapes retained GCC rows pr95822.C,
// pr95823.C and pr95824.C compile.

#include <coroutine>
#include <memory>

struct task
{
  struct promise_type
  {
    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void return_void() {}
    task get_return_object() { return {}; }
    void unhandled_exception() noexcept {}
  };

  bool await_ready() const noexcept { return false; }
  void await_suspend(std::coroutine_handle<>) noexcept {}
  void await_resume() noexcept {}
};

task g();
task g(int);

struct throwing_destructor
{
  ~throwing_destructor() noexcept(false) {}
};

task local_object_across_await()
{
  throwing_destructor error;
  co_await g();
}

struct id
{
  std::unique_ptr<int> value;
};

task smart_pointer_argument()
{
  std::unique_ptr<id> item;
  co_await g(*item->value);
}

struct factory
{
  virtual ~factory() = default;
  virtual int make_id() const;
};

task virtual_call_argument(factory &f)
{
  co_await g(f.make_id());
}
