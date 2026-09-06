// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
#include <future>
#include <functional>

int increment(int &value, int amount) { value += amount; return value; }
struct Object {
  int value;
  int add(int &other) { value += ++other; return value; }
  int read() const { return value; }
};
struct Callable {
  int calls;
  int operator()(int &value) { ++calls; return ++value; }
};

int main() {
  int value = 2;
  auto free_call = std::async(std::launch::async, &increment, std::ref(value), 3);
  if (free_call.get() != 5 || value != 5) return 1;
  Object object = {7};
  auto member_call = std::async(std::launch::async, &Object::add,
                               std::ref(object), std::ref(value));
  if (member_call.get() != 13 || value != 6 || object.value != 13) return 2;
  auto constant_call = std::async(std::launch::deferred, &Object::read, std::cref(object));
  if (constant_call.get() != 13) return 3;
  auto member_data = std::async(std::launch::deferred, &Object::value, std::ref(object));
  member_data.get() = 19;
  if (object.value != 19) return 4;
  Callable callable = {0};
  auto callable_call = std::async(std::launch::async, std::ref(callable), std::ref(value));
  if (callable_call.get() != 7 || callable.calls != 1 || value != 7) return 5;
  auto default_policy = std::async(&increment, std::ref(value), 2);
  return default_policy.get() != 9 || value != 9;
}
