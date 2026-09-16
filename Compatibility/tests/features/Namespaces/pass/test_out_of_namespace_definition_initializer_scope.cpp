// EXPECT_EXIT: 0
/* A definition written outside its namespace (`int outer::inner::value =
   seed;`) initializes in the scope of the object it defines. */
namespace outer {
  const int seed = 7;
  namespace inner {
    extern int value;
    extern int copy;
  }
}

int outer::inner::value = seed;
int outer::inner::copy(seed);

int main() {
  if (outer::inner::value != 7) return 1;
  if (outer::inner::copy != 7) return 2;
  return 0;
}
