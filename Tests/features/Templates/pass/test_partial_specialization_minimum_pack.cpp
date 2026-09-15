namespace sample {
template<class T, class... Args> struct Traits {
    typedef typename T::missing type;
};
}
template<class T, class U, class... Args> struct sample::Traits<T, U, Args...> {
    typedef int type;
};
struct Value {};
sample::Traits<Value, Value&, int>::type result;
int main() { return result; }
