static_assert(1'234 == 1234);
static_assert(0xA'F == 175);
static_assert(0b10'01 == 9);
static_assert(01'23 == 83);
static_assert(1'2.3'4e1'0 == 123400000000.0);
static_assert(0x1'A.F'0p1'0 == 27584.0);
static_assert(12'345ULL == 12345ULL);
static_assert(0xAB'CDu == 43981u);
static_assert(.1'25 == 0.125);
static_assert(1.2'5e-0'1 == 0.125);
static_assert(0xA'Bp-0'1 == 85.5);
int main() { return 1'234 != 1234; }