struct source {
  int value;
  constexpr operator int() const { return 42; }
};

struct target {
  source first;
  source second;
  int converted;
};

constexpr source value = {26};
constexpr target object = {4, value, value};

static_assert(object.first.value == 4, "aggregate scalar member");
static_assert(object.second.value == 26, "aggregate constexpr copy");
static_assert(object.converted == 42, "aggregate constexpr conversion");

int main() {
  return object.first.value != 4 || object.second.value != 26
      || object.converted != 42;
}

// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -std=c++14
