struct Value
{
  float number;

  float get() const { return number; }
  float operator[](const long long &index) const
  {
    return index == 0 ? number : 0.0f;
  }
};

int main()
{
  Value value;
  value.number = 1.0f;

  float fromMember = 0.0f;
  fromMember = value.get();
  if (fromMember != 1.0f)
    return 1;

  float fromIndex = 0.0f;
  fromIndex = value[0];
  return fromIndex == 1.0f ? 0 : 2;
}

// EXPECT_EXIT: 0
