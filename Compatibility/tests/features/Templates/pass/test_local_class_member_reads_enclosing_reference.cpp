/* An automatic reference whose referent has static storage is not frame
   state.  A member body of a local class is lowered after the enclosing
   frame is gone, so it has to keep naming the very same object. */

int counter;

struct Holder { int values[3]; };
Holder holder;

template<class T>
int bump ()
{
  T c = counter;
  struct A { static int step () { c++; return c; } };
  return A::step ();
}

int via_static ()
{
  static int slot;
  int &r = slot;
  struct A { static void f () { r += 5; } };
  A::f ();
  return slot;
}

int via_subobject ()
{
  int &r = holder.values[1];
  struct A { static void f () { r += 3; } };
  A::f ();
  return holder.values[1];
}

int main ()
{
  if (bump<int&> () != 1 || counter != 1) return 1;
  if (bump<int&> () != 2 || counter != 2) return 2;
  if (via_static () != 5) return 3;
  if (via_subobject () != 3 || holder.values[1] != 3) return 4;
  return 0;
}
