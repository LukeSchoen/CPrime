#include <new>

static int constructed;
static int destroyed;

struct Member {
  const Member *self;
  int value;
  Member(int n = 7) : self(this), value(n) { ++constructed; }
  ~Member() { if (self == this && value == 7) ++destroyed; }
};
struct Owner { Member first; Member tail[2]; };

static Owner globals[2];

static int valid(const Owner &owner) {
  return owner.first.self == &owner.first && owner.first.value == 7
         && owner.tail[0].self == &owner.tail[0] && owner.tail[0].value == 7
         && owner.tail[1].self == &owner.tail[1] && owner.tail[1].value == 7;
}

int main() {
  if (constructed != 6 || !valid(globals[0]) || !valid(globals[1])) return 1;
  {
    Owner local;
    if (constructed != 9 || !valid(local)) return 2;
  }
  if (destroyed != 3) return 3;
  union Storage { char bytes[sizeof(Owner)]; long long alignment; } storage;
  Owner *placed = new (storage.bytes) Owner();
  if (constructed != 12 || !valid(*placed)) return 4;
  placed->~Owner();
  return destroyed != 6;
}
