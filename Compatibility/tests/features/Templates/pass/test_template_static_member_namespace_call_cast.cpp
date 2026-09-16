#include <cmath>

template<typename T>
struct SignHelper {
    int value;

    static int Map(const T& x)
    {
        return 1 - 2 * (int)std::signbit((double)x);
    }
};

typedef SignHelper<double> DoubleSignHelper;

int main()
{
    DoubleSignHelper helper;
    helper.value = 1;
    if (helper.value != 1)
        return 1;
    return 0;
}
