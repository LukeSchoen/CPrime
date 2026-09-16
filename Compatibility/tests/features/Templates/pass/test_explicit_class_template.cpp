// Consolidated from similar standalone regressions; each cpc_case_N preserves one.

namespace cpc_case_0
{
template<typename T>
struct identity;

template<>
struct identity<int> {
    static int value() {
        return 7;
    }
};

int run() {
    return identity<int>::value() - 7;
}
}

namespace cpc_case_1
{
template<typename T>
struct identity;

template<>
struct identity<double> {
    static double value() {
        return 2.5;
    }
};

int run() {
    double v = identity<double>::value();
    return v == 2.5 ? 0 : 1;
}
}

namespace cpc_case_2
{
template<typename T, typename U>
struct PairValue {
    static int value() {
        return 1;
    }
};

template<>
struct PairValue<double, double> {
    static int value() {
        return 5;
    }
};

int run() {
    return PairValue<int, double>::value() + PairValue<double, double>::value() - 6;
}
}

int main()
{
  if (int code = cpc_case_0::run()) { return code; }
  if (int code = cpc_case_1::run()) { return code; }
  if (int code = cpc_case_2::run()) { return code; }
  return 0;
}
