struct DefaultedMembers
{
  DefaultedMembers();
  DefaultedMembers(const DefaultedMembers &other) = default;
  DefaultedMembers(DefaultedMembers &&other) = default;
  ~DefaultedMembers() = default;
  DefaultedMembers &operator=(const DefaultedMembers &other) = default;
  DefaultedMembers &operator=(DefaultedMembers &&other) = default;
  int value;
};

DefaultedMembers::DefaultedMembers() : value(0)
{
}

int main()
{
  DefaultedMembers value;
  return value.value;
}
