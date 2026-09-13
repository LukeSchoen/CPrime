/* Complex arithmetic, conversions and the real/imaginary part operators,
   reduced from the retired external corpus rows (g++.dg/ext/complex3.C,
   g++.dg/opt/complex3.C, complex5.C, complex6.C, conj1.C, conj2.C,
   g++.dg/other/complex1.C, g++.old-deja/g++.brendan/complex1.C,
   g++.dg/tree-ssa/pr50622.C). */

typedef __complex__ double cdouble;
typedef __complex__ float cfloat;

extern "C" void exit(int);

__complex__ double global_value;

struct Vector
{
  Vector & operator+=(const Vector & other)
  {
    theX += other.theX;
    theY += other.theY;
    return *this;
  }

  cdouble theX;
  cdouble theY;
};

static Vector operator+(Vector a, const Vector & b)
{
  return a += b;
}

struct Parts
{
  Parts(double real, double imaginary)
  {
    __real__ value = real;
    __imag__ value = imaginary;
  }

  explicit Parts(__complex__ float z)
  {
    __imag__ z = __imag__ z * 2.0f;
    value = z;
  }

  double real() const { return __real__ value; }
  double imaginary() const { return __imag__ value; }

  __complex__ double value;
};

static cdouble add(cdouble a, cdouble b) { return a + b; }

static inline cdouble real_to_complex(double real)
{
  cdouble z = 0.0;
  __real__ z = real;
  return z;
}

static cdouble guarded(cdouble x)
{
  try
  {
    try
    {
      x = x + 1.0;
    }
    catch (...)
    {
      x = x - 1.0;
    }
  }
  catch (...)
  {
  }
  return x;
}

int main()
{
  cdouble a = 1.0 + 2.0i;
  cdouble b = 3.0 + 4.0i;

  if (__real__ a != 1 || __imag__ a != 2) return 1;
  if (__real__ (a + b) != 4 || __imag__ (a + b) != 6) return 2;
  if (__real__ (b - a) != 2 || __imag__ (b - a) != 2) return 3;
  if (__real__ (a * b) != -5 || __imag__ (a * b) != 10) return 4;
  if (__real__ (b / a) != 2.2 || __imag__ (b / a) != -0.4) return 5;
  if (__real__ (-a) != -1 || __imag__ (-a) != -2) return 6;
  if (__real__ (+a) != 1 || __imag__ (+a) != 2) return 7;

  /* A real operand converts to the complex element type before the
     operation. */
  if (__real__ (a + 1) != 2 || __imag__ (a + 1) != 2) return 8;
  if (__real__ (a * 2) != 2 || __imag__ (a * 2) != 4) return 9;
  if (__real__ (a / 2.0) != 0.5 || __imag__ (a / 2.0) != 1) return 10;
  if (__real__ (2.0 + a) != 3 || __imag__ (2.0 + a) != 2) return 11;

  /* Compound assignment keeps the left operand's type. */
  cdouble c = a;
  c += b;
  if (__real__ c != 4 || __imag__ c != 6) return 12;
  c -= a;
  if (__real__ c != 3 || __imag__ c != 4) return 13;
  c *= a;
  if (__real__ c != -5 || __imag__ c != 10) return 14;
  c /= a;
  if (__real__ c != 3 || __imag__ c != 4) return 15;
  cfloat narrow = 2.0fi;
  narrow *= 2.0f;
  if (__imag__ narrow != 4) return 16;
  narrow += 1.0f;
  if (__real__ narrow != 1 || __imag__ narrow != 4) return 17;

  /* Equality compares both parts; `!` is zero-test. */
  if (!(a == a) || a != a) return 18;
  if (a == b || !(a != b)) return 19;
  if (!(!!a)) return 20;
  cdouble zero = 0i;
  if (zero != 0) return 21;
  if (!(!zero)) return 22;

  /* The part operators are lvalues. */
  __real__ global_value = 7;
  __imag__ global_value = 8;
  if (__real__ global_value != 7 || __imag__ global_value != 8) return 23;
  __real__ global_value += 1;
  __imag__ global_value *= 2;
  if (__real__ global_value != 8 || __imag__ global_value != 16) return 24;

  /* The real and imaginary builtins agree with the part operators. */
  if (__builtin_crealf(narrow) != 1) return 25;
  if (__builtin_cimagf(narrow) != 4) return 26;
  if (__builtin_creal(a) != 1 || __builtin_cimag(a) != 2) return 27;

  /* A complex value initializes a struct member as a whole. */
  struct Holder { __complex__ double c; };
  Holder held = {2 + 2i};
  if (__imag__ held.c != 2) return 28;
  int one = 1;
  Holder chosen = (one == 1) ? held : (Holder){3 + 3i};
  if (__imag__ chosen.c != 2) return 29;

  /* Class members keep complex identity through constructors and
     operators. */
  Parts parts(3, -4);
  if (parts.real() != 3 || parts.imaginary() != -4) return 30;
  cfloat source = 2.0 + 1.0i;
  Parts scaled(source);
  if (scaled.real() != 2 || scaled.imaginary() != 2) return 37;

  Vector first;  __real__ first.theX = 1; __imag__ first.theX = 2;
  Vector second; __real__ second.theX = 10; __imag__ second.theX = 20;
  first.theY = 3i;
  second.theY = 4.0 + 0i;
  Vector sum = first + second;
  if (__real__ sum.theX != 11 || __imag__ sum.theX != 22) return 31;
  if (__real__ sum.theY != 4 || __imag__ sum.theY != 3) return 32;

  if (__real__(add(a, b)) != 4 || __imag__(add(a, b)) != 6) return 33;

  /* A complex object reaches a function through a const pointer. */
  const cdouble *data = &a;
  cdouble walked = real_to_complex(0.0);
  walked += data[0];
  if (__real__ walked != 1 || __imag__ walked != 2) return 34;
  if (__real__ real_to_complex(4.5) != 4.5) return 35;

  /* Complex locals survive a nested try/catch. */
  if (__real__ guarded(a) != 2 || __imag__ guarded(a) != 2) return 36;

  exit(0);
}
