// EXPECT_EXIT: 0

struct Storage
{
  Storage() : marker(11) {}

  int marker;
};

struct Text
{
  Text(int input) { value = input; }

  Storage storage;
  int value;
};

struct Receiver
{
  Receiver() : result(0) {}

  void Accept(const Text &first, const Text &middle, const Text &last)
  {
    result = (first.value == 17 ? 0 : 1)
        | (middle.value == 29 ? 0 : 2)
        | (last.value == 43 ? 0 : 4)
        | (first.storage.marker == 11 ? 0 : 8)
        | (middle.storage.marker == 11 ? 0 : 16)
        | (last.storage.marker == 11 ? 0 : 32);
  }

  void Forward(const Text &first, const Text &last)
  {
    Accept(first, 29, last);
  }

  int result;
};

int main()
{
  Text first(17);
  Text last(43);
  Receiver receiver;
  receiver.Forward(first, last);
  return receiver.result;
}
