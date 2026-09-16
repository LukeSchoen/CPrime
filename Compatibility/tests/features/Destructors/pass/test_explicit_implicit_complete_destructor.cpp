// EXPECT_EXIT: 0
#include <new>

static int live;
static int order;
struct Member {
  int id;
  Member(int n = 1) : id(n) { ++live; }
  ~Member() { --live; order = order * 10 + id; }
};
struct Base { Member member; };
struct Complete : Base { Member members[2]; };

int main() {
  union Storage { unsigned char bytes[sizeof(Complete)]; long long alignment; } storage;
  Complete* object = new(storage.bytes) Complete;
  object->member.id = 1;
  object->members[0].id = 2;
  object->members[1].id = 3;
  if (live != 3) return 1;
  object->~Complete();
  if (live || order != 321) return 2;

  const Complete* immutable = new(storage.bytes) const Complete();
  immutable->~Complete();
  return live || order != 321111;
}
