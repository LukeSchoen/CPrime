// CL gap probe: lib_complex. <complex> is missing from the runtime.
#include <complex>

int main() {
  std::complex<double> value(1.0, 2.0);
  if (value.real() != 1.0) return 1;
  return value.imag() == 2.0 ? 0 : 2;
}
