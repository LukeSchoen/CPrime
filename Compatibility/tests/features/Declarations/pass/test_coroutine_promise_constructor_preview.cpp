// EXPECT_COMPILE_ARGS: -fcoroutines
// EXPECT_COMPILE_ONLY: 1

// GCC rows pr104981-preview-this.C, pr115550-preview-this.C and
// pr94682-preview-this.C.  The implicit promise is constructed from the
// coroutine object before the body runs, so base conversion, forwarding
// references and deduced template parameters reach the promise constructor.

#include <coroutine>
#include <type_traits>

struct base {};
struct promise;

struct result
{
  using promise_type = promise;
};

struct promise
{
  promise(const base&, auto&&...) {}
  result get_return_object() { return {}; }
  static std::suspend_never initial_suspend() { return {}; }
  static std::suspend_always final_suspend() noexcept { return {}; }
  static void unhandled_exception() { throw; }
  void return_value(int) {}
};

struct derived : base
{
  int member = 41;

  result run()
  {
    ++member;
    co_return member;
  }
};

template <typename T> struct remove_reference { using type = T; };
template <typename T> struct remove_reference<T&> { using type = T; };
template <typename T> struct remove_reference<T&&> { using type = T; };
template <typename T> using remove_reference_t = typename remove_reference<T>::type;
template <typename, typename> struct is_same { static constexpr bool value = false; };
template <typename T> struct is_same<T, T> { static constexpr bool value = true; };
template <typename T, typename U> concept same_as = is_same<T, U>::value;

struct deduced
{
  struct promise_type
  {
    template <typename Arg>
    explicit promise_type(Arg&&)
    {
      static_assert(same_as<remove_reference_t<remove_reference_t<Arg>>,
                            remove_reference_t<Arg>>);
    }

    deduced get_return_object() { return {}; }
    std::suspend_never initial_suspend() noexcept { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void return_void() {}
    void unhandled_exception() {}
  };

  deduced run() { co_return; }
};

struct future;
struct pointer_promise
{
  template <typename Class>
  pointer_promise(Class&, int)
  {
    static_assert(!std::is_pointer<Class>::value);
  }

  std::suspend_never initial_suspend() { return {}; }
  std::suspend_never final_suspend() noexcept { return {}; }
  future get_return_object() { return {}; }
  void return_value(int) {}
  void unhandled_exception() {}
};

struct future
{
  using promise_type = pointer_promise;
};

struct caller
{
  future run(int) { co_return 0; }
};
