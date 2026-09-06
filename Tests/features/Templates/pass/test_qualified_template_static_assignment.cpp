namespace library { namespace {
template<class T> struct Memory { static int value; };
template<class T> int Memory<T>::value = 1;
typedef Memory<int> Selected;
}}
int main() {
  library::Selected::value = 7;
  return library::Selected::value != 7;
}
