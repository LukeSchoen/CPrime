// EXPECT_EXIT: 0
// A using declaration imports the base class's static member functions.  The
// allocation functions are free-function overloads rather than instance
// members, so `new (p) type` used to report `no matching class allocation
// function` even though the derived class imported `allocator::operator new`.

struct allocator
{
  static void *operator new (__SIZE_TYPE__, void *p) { return p; }
  static void operator delete (void *) { }
};

struct type : public allocator
{
  using allocator::operator new;
  using allocator::operator delete;
};

int main ()
{
  alignas (type) unsigned char storage[sizeof (type)];
  type *p = new (storage) type;
  if ((void *) p != (void *) storage)
    return 1;
  return 0;
}
