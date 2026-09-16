template<typename T>
struct List
{
  T *data;

  List() : data(0) {}
  List(const List &o) = default;

  template<typename U>
  List(const List<U> &other) : data(0) { other.data; }
};

struct String
{
  List<char> data;

  String() = default;
  String(const String &o) = default;
  String &operator=(const String &rhs) = default;
};

int main()
{
  String first;
  String second(first);
  second = first;
  return 0;
}
