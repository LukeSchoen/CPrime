template<class T> struct Reference { typedef T& Type; };
template<class T> void update(const typename Reference<T>::Type value) { value = 1; }
template<> void update<int>(const typename Reference<int>::Type value) { value = 2; }
template<class T> int constant() { return sizeof(T); }
template int constant<int>(void);
int main() { int value = 0; update<int>(value); return value != 2 || constant<int>() != sizeof(int); }
