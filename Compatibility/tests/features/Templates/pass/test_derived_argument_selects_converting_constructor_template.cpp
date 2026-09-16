/* Deduction of a constructor template parameter spelled as the class's own
   template-id also searches the base classes of the argument, so a derived
   argument selects the converting constructor of its base specialization. */

template<class T>
struct Box
{
  T value;
  Box () : value (0) {}
  template<class U>
  Box (const Box<U> &other) : value ((T)sizeof (U)) {}
};

/* Padding keeps `sizeof (IntBox)` distinct from `sizeof (int)`, so the
   deduced argument cannot be confused with the derived argument type. */
struct IntBox : Box<int> { char padding[7]; };

template<class U>
int deduced_size (const Box<U> &) { return (int)sizeof (U); }

int main ()
{
  IntBox source;
  Box<float> converted (source);
  if (converted.value != 4.0f) return 1;
  if (deduced_size (source) != 4) return 2;
  return 0;
}
