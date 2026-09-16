// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
extern "C" void abort();
static int calls, destroyed, attempts;
struct Observe {
  ~Observe() { if (calls != 2 || destroyed != 4) abort(); }
} observe;
int next_value() { return ++calls * 4; }
struct Plain { int value; };
struct Aggregate {
  int value;
  ~Aggregate() { ++destroyed; }
};
const Plain& plain() {
  static const Plain value = {next_value()};
  return value;
}
const Aggregate* aggregate() {
  static const Aggregate values[2] = {{next_value()}, {11}};
  return values;
}
struct Member {
  const Member* self;
  int value;
  Member(int n): self(this), value(n) {}
  Member(const Member&) = delete;
  Member(Member&&) = delete;
  ~Member() { ++destroyed; }
};
struct Composite { Member member; int tail; };
int late() { if (++attempts == 1) throw 37; return 17; }
const Composite& composite() {
  static const Composite value = {Member(13), late()};
  return value;
}
int main() {
  if (plain().value != 4 || plain().value != 4) return 1;
  if (aggregate()[0].value != 8 || aggregate()[1].value != 11) return 2;
  if (calls != 2 || destroyed) return 3;
  try { composite(); return 4; } catch (int value) {
    if (value != 37 || destroyed != 1) return 5;
  }
  const Composite& value = composite();
  if (value.member.value != 13 || value.tail != 17
      || value.member.self != &value.member || &composite() != &value
      || attempts != 2 || destroyed != 1) return 6;
  return 0;
}
