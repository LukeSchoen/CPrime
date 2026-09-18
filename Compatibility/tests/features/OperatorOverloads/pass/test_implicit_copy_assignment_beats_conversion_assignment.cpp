// EXPECT_EXIT: 0
// Copying an object copies each member with the implicit memberwise operation.
// A member whose class declares only a converting assignment (`operator=(bool)`
// reached through a safe-bool conversion) used to win that copy: the member was
// overwritten with the conversion result instead of its own value, so the
// copied handle lost its pointer.  The same selection also corrupted copies
// materialized for by-value calls and constructor arguments.
typedef void (*safe_bool)(int ***);

static void dummy(int ***)
{
}

struct Handle
{
  int *pointer;

  Handle() : pointer(0) {}
  explicit Handle(int *value) : pointer(value) {}

  operator safe_bool() const { return pointer ? &dummy : 0; }
  bool operator!() const { return pointer == 0; }

  Handle &operator=(bool value)
  {
    pointer = value ? (int *)1 : (int *)0;
    return *this;
  }
};

struct Holder
{
  Handle handle;
  int value;
};

template <typename T>
struct Pair
{
  T first, second;

  Pair(T head, T tail) : first(head), second(tail) {}
};

struct Producer
{
  int *payload;

  Producer() : payload(0) {}

  Holder Head() const
  {
    Holder item;
    item.handle.pointer = payload;
    item.value = 0x4321;
    return item;
  }

  Holder Tail() const
  {
    Holder item;
    item.handle.pointer = 0;
    item.value = 0;
    return item;
  }

  Pair<Holder> Both() const { return Pair<Holder>(Head(), Tail()); }
};

static int observe(Holder copy)
{
  return copy.handle.pointer == (int *)0x1234 && copy.value == 0x4321 ? 0 : 1;
}

int main()
{
  Holder source;
  source.handle.pointer = (int *)0x1234;
  source.value = 0x4321;

  Holder assigned;
  assigned = source;
  if (assigned.handle.pointer != (int *)0x1234) return 1;
  if (assigned.value != 0x4321) return 2;

  Holder constructed(source);
  if (constructed.handle.pointer != (int *)0x1234) return 3;
  if (constructed.value != 0x4321) return 4;

  if (observe(source) != 0) return 5;

  int payload = 9;
  Handle direct;
  direct.pointer = &payload;
  Handle target;
  target = direct;
  if (target.pointer != &payload) return 6;

  /* Two by-value class arguments materialized for a template constructor, the
     shape that copied a safe-bool handle as a bool in an iterator wrapper. */
  Producer producer;
  producer.payload = (int *)0x1234;
  Pair<Holder> pair = producer.Both();
  if (pair.first.handle.pointer != (int *)0x1234) return 7;
  if (pair.second.handle.pointer != 0) return 8;
  return 0;
}
