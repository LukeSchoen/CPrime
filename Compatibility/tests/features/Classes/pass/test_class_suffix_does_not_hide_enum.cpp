typedef enum { ModeA, ModeB } WindowMode;
struct Prefix_WindowMode {
  WindowMode mode;
  Prefix_WindowMode() : mode(ModeB) {}
};
namespace Scope {
  struct Owner {
    struct Nested {
      int value;
      Nested() : value(42) {}
    };
  };
}
int main() {
  Prefix_WindowMode window;
  Scope::Owner::Nested nested;
  if (window.mode != ModeB) return 1;
  if (nested.value != 42) return 2;
  return 0;
}
