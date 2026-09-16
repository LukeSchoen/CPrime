class Text
{
public:
  Text(const char *value, int length) : first(value[0]), size(length) {}
  template<typename T> Text(const T &value) : first('?'), size(1) {}

  char first;
  int size;
};

Text MakeText(const char *value, int length)
{
  return Text((char *)value, length);
}

int main()
{
  Text text = MakeText("okay", 4);
  return text.first == 'o' && text.size == 4 ? 0 : 1;
}
