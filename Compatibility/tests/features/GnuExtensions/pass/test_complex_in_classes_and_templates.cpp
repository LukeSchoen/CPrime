/* Complex members inside classes, templates and variadic calls. */

#include <stdarg.h>

extern "C" void exit(int);

static double observed;
static void record(int value) { observed = value; }

namespace numbers
{
  template<typename _Tp> class complex;

  template<> class complex<double>
  {
  public:
    complex(double = 0.0, double = 0.0);

    double real() const { return __real__ value; }
    double imaginary() const { return __imag__ value; }

  private:
    typedef __complex__ double storage;
    storage value;
  };

  inline
  complex<double>::complex(double __r, double __i)
  {
    __real__ value = __r;
    __imag__ value = __i;
  }
}

template <int Dim, class T>
class Engine
{
public:
  Engine (T val = T()) { (void)val; }
};

struct Convertible
{
  operator int() { return 5; }
};

template <typename> class Record;

template <> struct Record<float>
{
  float foo () { return __real__ b; }
  _Complex double b;
};

template <class T>
void poke ()
{
  Record<T> h;
  h.b = 2.0 + 3.0i;
  T *a = (T *) &h;
  a[0] = a[1] = 6;
  h.foo () ? record(1) : record(2);
}

struct ComplexHolder
{
  __complex__ float a;
};

static double variadic_imaginary (int z, ...)
{
  struct ComplexHolder arg;
  va_list ap;
  va_start (ap, z);
  arg = va_arg (ap, struct ComplexHolder);
  va_end (ap);
  return __imag__ arg.a;
}

int main()
{
  /* A class-template specialization stores its value in a complex member. */
  numbers::complex<double> value (3.0, -4.0);
  if (value.real() != 3.0 || value.imaginary() != -4.0) return 1;

  /* The specialization is usable as a defaulted template argument. */
  Engine<1, numbers::complex<double> > engine;
  (void)engine;

  /* __real__ of a non-complex class applies its conversion to int. */
  if (__real__ Convertible() != 5) return 2;

  /* A template writes through a T* aliasing the complex member and reads its
     real part back. */
  poke<float>();
  if (observed != 1) return 3;

  /* A struct holding a complex member passes through a variadic call. */
  struct ComplexHolder holder;
  __real__ holder.a = 1.5f;
  __imag__ holder.a = 2.5f;
  if (variadic_imaginary (0, holder) != 2.5f) return 4;

  exit(0);
}
