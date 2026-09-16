template <typename A = int, typename B = int, typename C = int,
          typename D = int, typename E = int>
struct Selector { enum { value = 0 }; };

/* The partial's written argument list omits the primary's trailing default. */
template <typename A, typename B, typename C, typename D>
struct Selector<A, B, C, D> { enum { value = 1 }; };

int main()
{
  return Selector<float, float>::value != 1;
}
