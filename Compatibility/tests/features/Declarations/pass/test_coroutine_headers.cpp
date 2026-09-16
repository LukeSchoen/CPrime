// EXPECT_COMPILE_ARGS: -fcoroutines

#ifndef __cpp_impl_coroutine
#error coroutine language feature macro is missing
#endif
#if __cpp_impl_coroutine != 201902L
#error coroutine language feature macro has the wrong value
#endif

#include <coroutine>
#include <experimental/coroutine>

#ifndef __cpp_lib_coroutine
#error coroutine library feature macro is missing
#endif
#if __cpp_lib_coroutine != 201902L
#error coroutine library feature macro has the wrong value
#endif

int main()
{
  std::coroutine_handle<> standard;
  std::experimental::coroutine_handle<> experimental;
  std::suspend_never ready;
  std::suspend_always suspended;
  return (standard ? 1 : 0) | (experimental ? 2 : 0)
      | (ready.await_ready() ? 0 : 4) | (suspended.await_ready() ? 8 : 0);
}
