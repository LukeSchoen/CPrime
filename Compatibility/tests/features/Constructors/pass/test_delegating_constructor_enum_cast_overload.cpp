// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
struct Text {
  int value;
  explicit Text(int v): value(v) {}
};
struct Vector {
  int value;
  explicit Vector(int v): value(v) {}
};
enum Flags { Default=0, Fullscreen=1, OpenGL=2 };

struct Window {
  int sum;
  Window(const Vector&, const Text&, const Flags&);
  Window(const Text&, const Vector&, const Flags&);
  Window(const Text&, const bool&, const bool&, const Vector&, const Flags&);
};

Window::Window(const Text& title, const bool& gl, const bool& fullscreen,
               const Vector& size, const Flags& flags)
  : Window(title, size,
           Flags(flags | (fullscreen ? Fullscreen : 0) | (gl ? OpenGL : 0))) {}
Window::Window(const Vector& size, const Text& title, const Flags& flags)
  : Window(title, size, Flags(flags)) {}
Window::Window(const Text& title, const Vector& size, const Flags& flags)
  : sum(title.value + size.value + flags) {}

int category(const Flags&) { return 1; }
int category(unsigned int) { return 2; }

int main() {
  Window first(Text(5), true, true, Vector(7), Default);
  Window second(Vector(9), Text(3), Default);
  if (first.sum != 15 || second.sum != 12) return 1;
  if (category(Flags(1)) != 1 || category(Flags()) != 1) return 2;
  return category(1u) != 2;
}
