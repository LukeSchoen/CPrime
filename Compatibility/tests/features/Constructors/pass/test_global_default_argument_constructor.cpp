static int seed = 41;
struct Counter {
  int value;
  explicit Counter(int x = seed + 1) : value(x) { ++seed; }
};
static Counter first;
namespace Scope { Counter second; }
int main() {
  if (first.value != 42) return 1;
  if (Scope::second.value != 43) return 2;
  if (seed != 43) return 3;
  return 0;
}
