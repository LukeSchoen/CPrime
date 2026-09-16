static int cleanups;
struct Cleanup {
  ~Cleanup() { ++cleanups; }
};
struct Small {
  int value;
  Small() = default;
  ~Small() = default;
};
struct Large {
  long long first, second;
};
const Small small_value() {
  Cleanup cleanup;
  Small value;
  value.value = 17;
  return value;
}
const Large large_value() {
  Cleanup cleanup;
  Large value = {23, 29};
  return value;
}
int main() {
  const Small small = small_value();
  if (small.value != 17 || cleanups != 1) return 1;
  const Large large = large_value();
  return large.first != 23 || large.second != 29 || cleanups != 2;
}
