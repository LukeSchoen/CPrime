#include <initializer_list>

template<typename T>
struct List
{
  int marker;

  List() : marker(0) {}
  List(const std::initializer_list<T> &values) : marker(values.size() ? 99 : 98) {}

  List &operator=(const List &other)
  {
    marker = other.marker;
    return *this;
  }
};

struct String
{
  List<char> data;

  String() = default;
  String &operator=(const String &rhs) = default;
};

int main()
{
  String source;
  String destination;
  source.data.marker = 7;
  destination = source;
  return destination.data.marker == 7 ? 0 : 1;
}

// EXPECT_EXIT: 0
