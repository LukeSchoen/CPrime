/* An overload set argument associates every member of the set, so a member
   declared in a namespace reaches that namespace's overloads of the called
   name; the destination signature then selects the member to pass. */

static int calls;

namespace N
{
struct Tag { };
void pick (float) { calls += 1; }
void sink (void (*target) (float)) { calls += 10; target (1.0f); }
}

using N::pick;
template<class T> void pick (N::Tag, T) { }

void sink (void (*target) (int)) { calls += 100; target (1); }

int main ()
{
  calls = 0;
  sink (&pick);
  if (calls != 11)
    return 1;
  calls = 0;
  sink (pick);
  if (calls != 11)
    return 2;
  return 0;
}
