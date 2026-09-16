// EXPECT_EXIT: 0
struct Base {};
struct Left : Base {};
struct Right : Base {};
struct Derived : Left, Right {};
template<class T> T create();
template<class T> char (&select(char (*)[sizeof(static_cast<T>(create<Derived*>()))]))[1];
template<class T> char (&select(...))[2];
int main() {
    return sizeof(select<int>(0)) != 2 || sizeof(select<Base*>(0)) != 2
        || sizeof(select<void*>(0)) != 1;
}
