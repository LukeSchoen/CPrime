// EXPECT_EXIT: 0
template<class T> struct Base { int value; int get() { return value; } };
template<class T> struct Derived : Base<T> {};
template<class T> int inspect(Derived<T> &object) {
    object.template Derived<T>::template Base<T>::value = 23;
    return object.template Derived<T>::template Base<T>::get()
        + object.template Base<T>::template Base<T>::get();
}
int main() { Derived<int> object; return inspect(object) != 46; }
