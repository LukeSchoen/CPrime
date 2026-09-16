namespace geometry {
struct Outer {
  struct Fragment { int value; };
  struct Middle {
    using Value = int;
    struct Scan {
      Scan(const Fragment& fragment, Value Value);
      int result;
    };
  };
};
Outer::Middle::Scan::Scan(const Fragment& fragment, Value Value)
    : result(fragment.value + Value) {}
}

extern "C" {
struct FirstElement { int value; };
struct SecondElement { double value; };
struct FirstOwner { typedef FirstElement elem_type; elem_type value; };
struct SecondOwner { typedef SecondElement elem_type; elem_type value; };
}

struct Scalar {
  using Value = int;
  Scalar(Value Value);
  int value;
};
Scalar::Scalar(Value Value) : value(Value) {}

int main() {
  geometry::Outer::Fragment fragment = {7};
  geometry::Outer::Middle::Scan scan(fragment, 5);
  FirstOwner first = {{3}};
  SecondOwner second = {{4.5}};
  Scalar scalar(11);
  return scan.result != 12 || first.value.value != 3
      || second.value.value != 4.5 || scalar.value != 11;
}
