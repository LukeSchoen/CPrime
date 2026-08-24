template<typename T>
struct MoveValue
{
  T value;
};

template<typename T>
int consume_move(MoveValue<T> &&value)
{
  value.value += (T)4;
  return (int)value.value;
}

int main()
{
  MoveValue<int> value;
  value.value = 7;
  if (consume_move(static_cast<MoveValue<int>&&>(value)) != 11)
    return 1;
  return value.value == 11 ? 0 : 2;
}
