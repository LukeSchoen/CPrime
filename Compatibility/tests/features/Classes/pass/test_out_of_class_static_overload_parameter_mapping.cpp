struct StaticPayload
{
  int value;
};

class StaticMapping
{
public:
  static int Read(const StaticPayload &payload, int offset);
  static int Read(int value);
};

int StaticMapping::Read(const StaticPayload &payload, int offset)
{
  return payload.value + offset;
}

int StaticMapping::Read(int value)
{
  return value;
}

int main()
{
  StaticPayload payload;
  payload.value = 5;
  return StaticMapping::Read(payload, 3) == 8 ? 0 : 1;
}
