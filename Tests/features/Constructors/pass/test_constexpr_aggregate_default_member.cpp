struct aggregate {
  int index;
  const char *text;
  int padding;
  int value = text[index];
};

aggregate runtime = {1, "asdf"};
constexpr aggregate constant = {1, "asdf"};

static_assert(constant.index == 1, "explicit member");
static_assert(constant.text[0] == 'a' && constant.text[3] == 'f', "string member");
static_assert(constant.padding == 0 && constant.value == 's', "default member");

int main() {
  return runtime.index != 1 || runtime.text[3] != 'f' || runtime.padding != 0
      || runtime.value != 's';
}

// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -std=c++14
