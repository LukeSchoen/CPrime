namespace std {
    int signbit(double value)
    {
        return value < 0;
    }
}

#define MAP_LIKE(name, floatMap, doubleMap) \
template<typename T> struct _##name##Helper { static inline auto Map(const T& x) { return floatMap((float)x); } }; \
template<> struct _##name##Helper<double> { static inline auto Map(const double& x) { return doubleMap((double)x); } }; \
template<typename T> auto name(const T& x) { return _##name##Helper<T>::Map(x); } \

MAP_LIKE(signLike, 1 - 2 * (int)std::signbit, 1 - 2 * (int)std::signbit);

int main()
{
    return 0;
}
