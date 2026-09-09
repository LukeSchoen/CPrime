int choose(int) { return 0; }
struct Hidden {
    friend int choose(char) { return 1; }
    friend int associated(Hidden) { return 2; }
};
struct Later {
    friend int later(char) { return 3; }
};
int later(int) { return 4; }
int declared(char);
struct Existing {
    friend int declared(char) { return 5; }
};
int main() {
    return choose('a') != 0 || associated(Hidden()) != 2
        || later('a') != 4 || declared('a') != 5;
}
