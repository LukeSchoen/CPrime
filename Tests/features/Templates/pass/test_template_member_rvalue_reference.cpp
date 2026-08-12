// EXPECT_EXIT: 0

template<typename T>
class Sink
{
public:
  T stored;

  Sink()
  {
    stored = 0;
  }

  void move_from(T &&value)
  {
    stored = value + 3;
  }
};

int main(void)
{
  Sink<int> sink;
  int value;
  value = 4;
  sink.move_from(value);
  return sink.stored == 7 ? 0 : 1;
}
