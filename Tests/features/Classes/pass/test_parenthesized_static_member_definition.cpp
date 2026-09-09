// EXPECT_EXIT: 0
int function() { return 99; }
struct Owner {
    static int function() { return 7; }
    static int (*handler)();
    static int (*handlers[2])();
    static void empty() {}
    static void (*empty_handler)();
    int member() { return 13; }
    static int (Owner::*method)();
};
int early;
struct Check {
    Check() {
        early = Owner::handler == Owner::function
             && Owner::empty_handler == Owner::empty;
    }
} check;
int (*Owner::handler)() = function;
int (*Owner::handlers[2])() = { function, function };
void (*Owner::empty_handler)() = empty;
int (Owner::*Owner::method)() = &Owner::member;
int (*plain)() = function;
int main() {
    Owner object;
    return !early || Owner::handler() != 7 || Owner::handlers[1]() != 7
        || plain() != 99 || (object.*Owner::method)() != 13;
}
