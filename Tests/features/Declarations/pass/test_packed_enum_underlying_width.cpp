enum Small { small = 255 } __attribute__((packed));
enum __attribute__((packed)) Signed { low = -128, high = 127 };
enum Wide { wide = 256 } __attribute__((packed));
enum SignedWide { negative = -129 } __attribute__((packed));
enum Larger { larger = 65536 } __attribute__((packed));
enum Fixed : unsigned int { fixed = 1 } __attribute__((packed));
int main() {
    Small a = small;
    Signed b = low;
    Wide c = wide;
    SignedWide d = negative;
    return sizeof(a) != 1 || sizeof(small) != 1 || a != 255
        || sizeof(b) != 1 || sizeof(low) != 1 || b != -128
        || sizeof(c) != 2 || c != 256 || sizeof(d) != 2 || d != -129
        || sizeof(Larger) != 4 || sizeof(Fixed) != 4;
}
