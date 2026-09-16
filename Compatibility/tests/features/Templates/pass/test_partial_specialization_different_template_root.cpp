template<class T> struct Box { T value; };
template<class T> struct Identity { static T get() { return T(0); } };
template<class T> struct Identity<Box<T>> {
    static Box<T> get() { Box<T> result; result.value = 0; return result; }
};
int main() { return Identity<double>::get() != 0.0; }
