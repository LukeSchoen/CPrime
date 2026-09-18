// EXPECT_EXIT: 0
// A member function template returning `F *` instantiated with a function type
// has to produce a function pointer: `drawFn = e.GetFunction<void()>("scene");`
// against `template <typename F> F *GetFunction(const char *name)`.
struct Loader
{
  template <typename F> F *GetFunction() { return nullptr; }
};

void (*drawFn)() = nullptr;

int main()
{
  Loader loader;
  drawFn = loader.GetFunction<void()>();
  return drawFn ? 1 : 0;
}
