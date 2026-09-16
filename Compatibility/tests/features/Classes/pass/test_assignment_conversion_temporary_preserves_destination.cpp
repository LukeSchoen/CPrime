struct Member
{
  long long padding0;
  long long padding1;
  int value;

  Member() : padding0(0), padding1(0), value(0) {}

  Member &operator=(const Member &other)
  {
    value = other.value;
    return *this;
  }
};

struct Text
{
  Member member;

  Text() = default;

  Text(const char *)
  {
    member.value = 42;
  }

  Text &operator=(const Text &other) = default;
};

Text destination;

Text assign_and_return()
{
  destination = "converted";
  return destination;
}

int main()
{
  Text result = assign_and_return();
  return result.member.value == 42 ? 0 : 1;
}

// EXPECT_EXIT: 0
