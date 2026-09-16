namespace compatibility
{
class Utility
{
public:
  static int Convert(int value);
  static int Convert(const char* value);
  static char* Identity(char* value);
  static int Which(char* value, int* state);
  static int Which(const char* value, int* state);
  int Mixed(const char* value);
  static int Mixed(int* value);
};

class Consumer
{
public:
  Consumer();
  int Result();
  char* Text();

private:
  char text_[2];
};

int Utility::Convert(int value)
{
  return value;
}

int Utility::Convert(const char* value)
{
  return value[0] == 'a' ? 3 : 0;
}

int Utility::Which(char* value, int* state)
{
  return value[0] == 'q' && state == 0 ? 1 : 0;
}

int Utility::Which(const char* value, int* state)
{
  return value[0] == 'q' && state == 0 ? 2 : 0;
}

int Utility::Mixed(const char* value)
{
  return value[0] == 'm' ? 4 : 0;
}

int Utility::Mixed(int* value)
{
  return *value;
}

char* Utility::Identity(char* value)
{
  return value;
}

Consumer::Consumer()
{
  text_[0] = 'q';
  text_[1] = 0;
}

int Consumer::Result()
{
  int mixed = 2;
  Utility utility;
  return Utility::Convert(5) + Utility::Convert("abc")
         + Utility::Which(text_, 0) + utility.Mixed("mixed")
         + Utility::Mixed(&mixed);
}

char* Consumer::Text()
{
  return Utility::Identity(text_);
}
}

int main()
{
  compatibility::Consumer consumer;
  return consumer.Result() == 15 && consumer.Text()[0] == 'q' ? 0 : 1;
}
