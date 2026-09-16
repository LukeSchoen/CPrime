// EXPECT_EXIT: 0
int main() {
    int value = 0x1234;
    int const* constant = &value;
    int* pointer = const_cast<int*>(constant);
    int& reference = const_cast<int&>(*constant);
    unsigned long long address = reinterpret_cast<unsigned long long>(pointer);
    void* opaque = reinterpret_cast<void*>(pointer);
    unsigned char& byte = reinterpret_cast<unsigned char&>(value);
    return pointer != &value || &reference != &value
        || reinterpret_cast<int*>(address) != &value
        || static_cast<int*>(opaque) != &value || byte != 0x34;
}
