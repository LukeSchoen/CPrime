#include <limits>

int main() {
    float f = std::numeric_limits<float>::infinity();
    double d = std::numeric_limits<double>::max();
    int i = std::numeric_limits<int>::max();

    return (f > 0.0f && d > 1.0 && i > 0) ? 0 : 1;
}
