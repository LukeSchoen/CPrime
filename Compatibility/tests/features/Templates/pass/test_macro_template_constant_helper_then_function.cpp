#define CONSTANT_LIKE(name, floatValue, doubleValue) template<typename T> \
inline auto name(); \
template<typename T> \
struct _##name##Helper \
{ \
    static inline float Value() { return (float)(doubleValue); } \
}; \
template<> \
struct _##name##Helper<double> \
{ \
    static inline double Value() { return (double)(doubleValue); } \
}; \
template<> \
struct _##name##Helper<float> \
{ \
    static inline float Value() { return (float)(floatValue); } \
}; \
template<typename T> \
inline auto name() \
{ \
    return _##name##Helper<T>::Value(); \
} \

CONSTANT_LIKE(epsilonLike, 1.E-6, 1.E-6)

int main()
{
    return 0;
}
