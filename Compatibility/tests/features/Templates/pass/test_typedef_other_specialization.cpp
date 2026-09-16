template<class T> struct Tuple { typedef Tuple<int> tail; };
template<> struct Tuple<int> { int marker; };
template<class T> struct Length { static const int value = Length<typename Tuple<T>::tail>::value; };
template<> struct Length<Tuple<int> > { static const int value = 1; };
int main() {
    Tuple<double>::tail terminal = {7};
    return terminal.marker != 7 || Length<Tuple<const int> >::value != 1;
}
