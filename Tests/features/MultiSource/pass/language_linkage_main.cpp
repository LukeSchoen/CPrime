int ordered(double);
int ordered(int);
int same_name(int, int);
int same_name_int(int);
enum Choice { selected = 3 };
int enum_argument(Choice);
typedef int (*Callback)(int);
int callback_argument(Callback, int);
int array_argument(int (&)[3]);
struct LinkageOwner { int value; };
int member_argument(LinkageOwner &, int LinkageOwner::*);
typedef struct { int first, second; } UnnamedRecord;
typedef UnnamedRecord LaterAlias;
int unnamed_argument(const LaterAlias &);
namespace Qualified {
int ordered(double);
int ordered(int);
}
int nested(double);
int nested(int);
int restored_cpp(double);
int restored_cpp(int);
extern "C" { int from_c(void); int c_defined(int); }
int main() {
  int (*address)(int) = same_name_int;
  int values[3] = {1, 2, 3};
  LinkageOwner owner = {17};
  LaterAlias unnamed = {20, 21};
  return ordered(1) != 11 || ordered(2.0) != 22
      || same_name(3, 4) != 47 || address(5) != 35
      || Qualified::ordered(6) != 56 || Qualified::ordered(7.0) != 67
      || nested(8) != 108 || nested(9.0) != 119
      || restored_cpp(10) != 140 || restored_cpp(11.0) != 151
      || !from_c() || c_defined(12) != 162
      || enum_argument(selected) != 163
      || callback_argument(address, 4) != 34
      || array_argument(values) != 4
      || member_argument(owner, &LinkageOwner::value) != 17
      || unnamed_argument(unnamed) != 41;
}
