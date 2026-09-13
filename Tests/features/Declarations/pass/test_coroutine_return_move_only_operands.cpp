// EXPECT_COMPILE_ARGS: -fcoroutines
// EXPECT_COMPILE_ONLY: 1

// GCC row co-return-syntax-10-movable.C.  A co_return operand that is a
// by-value parameter, an rvalue-reference parameter or an automatic object is
// an implicitly movable rvalue, so a promise with only return_value(T&&)
// accepts all three.

#include <coroutine>
#include <utility>

struct move_only
{
  int value;

  explicit move_only(int v) : value(v) {}
  move_only(move_only&& other) noexcept
      : value(std::exchange(other.value, -1))
  {
  }
  move_only& operator=(move_only&& other) noexcept
  {
    value = std::exchange(other.value, -1);
    return *this;
  }
  move_only(const move_only&) = delete;
  move_only& operator=(const move_only&) = delete;
  ~move_only() { value = -2; }
};

struct task
{
  struct promise_type
  {
    move_only value{0};

    task get_return_object() { return {}; }
    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void return_value(move_only&& v) noexcept { value = std::move(v); }
    void return_value(const move_only&) = delete;
    void unhandled_exception() {}
  };
};

task choose(bool by_value_branch, bool reference_branch,
            move_only by_value, move_only&& by_reference)
{
  move_only local(10);
  if (by_value_branch)
    co_return by_value;
  else if (reference_branch)
    co_return by_reference;
  else
    co_return local;
}
