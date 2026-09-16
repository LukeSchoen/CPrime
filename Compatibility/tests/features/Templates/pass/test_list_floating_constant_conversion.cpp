constexpr float rounded() { float value{0.1}; return value; }
constexpr float tiny() { float value{1e-300}; return value; }
static_assert(rounded() == 0.1f);
static_assert(tiny() == 0.0f);
double widen(float source) { double value{source}; return value; }
int main() { float value{0.1}; return value != 0.1f || widen(1.5f) != 1.5; }
