template<class T, class Result = T> struct Distance {
  using DistanceType = Result;
  template<class U, class V>
  DistanceType difference(const U a, const V b, unsigned long long) const {
    return (a - b) * (a - b);
  }
};
int main() {
  Distance<float, double> precise;
  Distance<int, int> integral;
  return precise.difference(3.5f, 1.0f, 0) != 6.25
      || integral.difference(7, 4, 0) != 9;
}
