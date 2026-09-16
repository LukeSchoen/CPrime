template<class T> int target(T) { return T::missing; }
inline int unused() {
    using Result = decltype(target(1));
    using Pointer = decltype(&target<int>);
    static_assert(sizeof(static_cast<int (*)(int)>(target<int>)) == sizeof(Pointer));
    return sizeof(target(1)) + sizeof(Result) + sizeof(Pointer);
}
int main() {}
