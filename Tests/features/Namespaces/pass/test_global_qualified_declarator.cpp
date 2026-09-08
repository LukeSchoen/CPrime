// EXPECT_EXIT: 0
void global_function();
template<class T> int read(T value) { return value.member; }
class Friend {
    int member;
    friend int ::read(Friend);
public:
    Friend() : member(29) {}
};
struct Value;
namespace Scope {
    extern Value value;
}
struct Value { int member; } ::Scope::value = {23};
namespace Scope {
    struct Local { friend void ::global_function(); };
}
int result;
void global_function() { result = Scope::value.member; }
int main() {
    class Local { friend void ::global_function(); };
    global_function();
    return result != 23 || read(Friend()) != 29;
}
