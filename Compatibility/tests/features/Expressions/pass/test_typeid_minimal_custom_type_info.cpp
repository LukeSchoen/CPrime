// EXPECT_COMPILE_ONLY: 1

namespace std {
class type_info {};
}

std::type_info first = typeid(int);

int probe()
{
  const std::type_info *identity = &typeid(void);
  return identity == &typeid(void) ? 0 : 1;
}
