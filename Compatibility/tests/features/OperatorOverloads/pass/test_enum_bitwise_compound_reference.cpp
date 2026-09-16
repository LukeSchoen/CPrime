enum Flags { A = 2, B = 16 };
constexpr Flags operator^(Flags a, Flags b) { return Flags((unsigned long long)a ^ (unsigned long long)b); }
Flags& operator^=(Flags& a, Flags b) { return a = a ^ b; }
int main() { Flags value = A; value ^= B; return value != 18; }
