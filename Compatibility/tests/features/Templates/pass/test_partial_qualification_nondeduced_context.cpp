template<class T> struct Void { typedef void type; };
template<class T, class = void> struct Select;
template<class T> struct Select<T, typename Void<typename T::type>::type> {
  enum { value = 3 };
};
template<class T> struct Select<T const, typename Void<typename T::type>::type> {
  enum { value = 7 };
};
struct Element { typedef int type; };
template<class T> struct Array;
template<class T, unsigned N> struct Array<T[N]> { enum { value = 11 }; };
template<class T, unsigned N> struct Array<const T[N]> { enum { value = 13 }; };
int main() { return Select<Element>::value != 3 || Select<Element const>::value != 7
                 || Array<int const[2]>::value != 13 || Array<int[2]>::value != 11; }
