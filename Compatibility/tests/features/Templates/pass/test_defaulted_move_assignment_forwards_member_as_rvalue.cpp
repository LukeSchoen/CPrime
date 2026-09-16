template<typename T>
struct MoveOnly
{
  T value;

  MoveOnly() : value(0) {}
  MoveOnly(const MoveOnly &) = delete;
  MoveOnly &operator=(const MoveOnly &) = delete;

  MoveOnly &operator=(MoveOnly &&other)
  {
    value = other.value;
    other.value = 0;
    return *this;
  }
};

struct Owner
{
  MoveOnly<int> member;

  Owner() = default;
  Owner &operator=(Owner &&other) = default;
};

int main()
{
  Owner source;
  Owner destination;
  source.member.value = 17;
  destination = static_cast<Owner &&>(source);
  return destination.member.value == 17 && source.member.value == 0 ? 0 : 1;
}

// EXPECT_EXIT: 0
