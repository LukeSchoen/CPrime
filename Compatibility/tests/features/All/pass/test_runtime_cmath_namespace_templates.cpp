#include <cmath>

int main() {
    float nan_value = 0.0f / 0.0f;
    double inf_value = 1.0 / 0.0;

    if (!std::isnan<float>(nan_value))
        return 1;
    if (!std::isinf<double>(inf_value))
        return 2;
    if (std::signbit<float>(1.0f))
        return 3;
    return 0;
}
