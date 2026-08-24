struct Payload
{
  long long first;
  long long second;
  long long third;

  Payload() : first(0), second(0), third(0) {}
  Payload(const Payload &) = default;
};

struct Value
{
  Payload payload;

  Value() = default;
  Value(const Value &) = default;
};

struct Owner
{
  int prefix;
  Value member;

  Owner() : prefix(11), member() {}

  Value get() const
  {
    return member;
  }
};

int main()
{
  Owner owner;
  owner.member.payload.first = 73;
  owner.member.payload.second = 91;
  owner.member.payload.third = 117;
  Value copied = owner.get();
  return copied.payload.first == 73
      && copied.payload.second == 91
      && copied.payload.third == 117 ? 0 : 1;
}

// EXPECT_EXIT: 0
