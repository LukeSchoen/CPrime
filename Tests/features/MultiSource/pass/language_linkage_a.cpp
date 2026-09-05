// The caller deliberately sees the overload declarations in the other order.
int ordered(int value) { return value + 10; }
int ordered(double value) { return (int)value + 20; }
int same_name_int(int value) { return value + 30; }
int same_name(int left, int right) { return left + right + 40; }
enum Choice { selected = 3 };
int enum_argument(Choice value) { return (int)value + 160; }
int callback_argument(int (*call)(int), int value) { return call(value); }
int array_argument(int (&values)[3]) { return values[0] + values[2]; }
struct LinkageOwner { int value; };
int member_argument(LinkageOwner &owner, int LinkageOwner::*member) {
  return owner.*member;
}
typedef struct { int first, second; } UnnamedRecord;
int unnamed_argument(const UnnamedRecord &value) {
  return value.first + value.second;
}

namespace Qualified {
int ordered(int value) { return value + 50; }
int ordered(double value) { return (int)value + 60; }
extern "C" int c_in_namespace(int value) { return value + 70; }
}

extern "C" int inherited_c(int);
int inherited_c(int value) { return value + 80; }

extern "C" {
int outer_c(int value) { return value + 90; }
extern "C++" {
int nested(int value) { return value + 100; }
int nested(double value) { return (int)value + 110; }
}
int restored_c(int value) { return value + 120; }
}

int restored_cpp(int value) { return value + 130; }
int restored_cpp(double value) { return (int)value + 140; }
