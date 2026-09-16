// EXPECT_EXIT: 0
typedef double Item;
union Owner {
    typedef char Item;
    struct Nested { Item value; int size(); };
    int storage;
};
int Owner::Nested::size() { Item local = 3; return sizeof(local); }
int main() {
    Owner::Nested object;
    return object.size() != 1 || sizeof(object.value) != 1;
}
