// EXPECT_COMPILE_ARGS: -Werror
int calls[2];

/* Address of an overload set with explicit template arguments: a sibling
   candidate that the written argument can bind completely must not win, and
   the target function type selects the specialization. */
template <class T> void entry(T*);
template <class T, class U> void entry(T*, U) { calls[0] = calls[0] + 1; }

template <class T, class U>
void call_entry(void (*function)(T, U), U) { function((T)0, (U)1); }

/* A single candidate whose parameter list outruns the written arguments is
   bound by the destination signature alone. */
template <class T, class U> void tail(T*, U) { calls[1] = calls[1] + 1; }

template <class T, class U>
void call_tail(void (*function)(T, U), U) { function((T)0, (U)1); }

int main() {
  call_entry<int*>(&entry, 1);
  call_entry<int*>(&entry<int>, 1);
  call_entry<int*>(entry, 1);
  call_entry<int*>(entry<int>, 1);
  if (calls[0] != 4) return 1;

  call_tail<int*>(&tail<int>, 1);
  if (calls[1] != 1) return 2;
  return 0;
}
