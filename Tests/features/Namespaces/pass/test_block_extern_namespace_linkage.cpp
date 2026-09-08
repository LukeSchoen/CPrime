namespace Named {
void write() {
  extern int value;
  extern int get();
  value = get();
}
int value;
int get() { return 31; }
}
namespace {
void update() { extern int hidden; hidden = 19; }
int hidden;
}
namespace CLinkage {
extern "C" int shared;
void write() { shared = 17; }
}
int shared;
int main() {
  Named::write();
  update();
  CLinkage::write();
  return Named::value == 31 && hidden == 19 && shared == 17 ? 0 : 1;
}
