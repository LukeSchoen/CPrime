#pragma once
namespace member_linkage {
class Service {
  static int private_value(int);
protected:
  static int protected_value(short);
public:
  static int value(int);
  static double value(double);
  struct Nested_Type {
    static int value(int);
  };
};
}
