/* _Complex and __complex__ declarations, element types, layout and imaginary
   constants, reduced from the retired external corpus rows
   (g++.dg/ext/complex1.C, complex2.C, complex5.C, complex6.C,
   g++.dg/init/complex1.C, g++.dg/opt/complex1.C, complex2.C, complex4.C).

   A complex type is the aggregate of its real and imaginary parts: the size
   is twice the element size, the alignment is the element's, and a
   functional-style conversion or an imaginary constant produces a whole
   value. */

typedef float __complex__ fcomplex;
typedef __complex__ double cdouble;
typedef __complex__ long double lcomplex;
typedef _Complex int icomplex;
typedef _Complex float cfloat;
typedef __complex__ short scomplex;
typedef __complex__ char ccomplex;
typedef __complex__ unsigned int ucomplex;

extern "C" void exit(int);

/* Namespace-scope complex objects initialized from a functional conversion
   are constants; the two spellings name the same type. */
fcomplex global_cast_default = fcomplex();
fcomplex global_cast_zero = fcomplex(0);
cdouble global_imaginary = 2.0i;

static _Complex float unchanged(_Complex float z) { return z; }
static cdouble double_from_int() { return 1; }
static cfloat float_value() throw() { return 2.0fi; }
static int overloaded(_Complex int) { return 1; }
static int overloaded(double) { return 2; }

int main()
{
  if (sizeof(fcomplex) != 2 * sizeof(float)) return 1;
  if (sizeof(cdouble) != 2 * sizeof(double)) return 2;
  if (sizeof(lcomplex) != 2 * sizeof(long double)) return 3;
  if (sizeof(icomplex) != 2 * sizeof(int)) return 4;
  if (sizeof(cfloat) != sizeof(fcomplex)) return 5;
  if (sizeof(scomplex) != 2 * sizeof(short)) return 28;
  if (sizeof(ccomplex) != 2 * sizeof(char)) return 29;
  if (sizeof(ucomplex) != 2 * sizeof(unsigned int)) return 30;

  if (__real__ global_cast_default != 0) return 6;
  if (__imag__ global_cast_default != 0) return 7;
  if (__real__ global_cast_zero != 0) return 8;
  if (__imag__ global_cast_zero != 0) return 9;
  if (__imag__ global_imaginary != 2) return 10;
  if (__real__ global_imaginary != 0) return 11;

  /* `new T()` value-initializes a complex object. */
  void *heap = new __complex__ int ();
  if (heap == 0) return 12;
  delete (int *)heap;

  /* Each imaginary spelling keeps its magnitude in the imaginary part. */
  fcomplex imag_float = 2.0fi;
  if (__real__ imag_float != 0 || __imag__ imag_float != 2) return 13;
  cdouble imag_double = 2.0i;
  if (__real__ imag_double != 0 || __imag__ imag_double != 2) return 14;
  lcomplex imag_long = 3.0Li;
  if (__real__ imag_long != 0 || __imag__ imag_long != 3) return 15;
  icomplex imag_int = 90i;
  if (__real__ imag_int != 0 || __imag__ imag_int != 90) return 16;

  /* A real operand converts to the corresponding complex type. */
  cdouble sum = 1.0 + 90i;
  if (__real__ sum != 1 || __imag__ sum != 90) return 17;
  cdouble from_int = double_from_int();
  if (__real__ from_int != 1 || __imag__ from_int != 0) return 18;
  cfloat from_call = float_value();
  if (__real__ from_call != 0 || __imag__ from_call != 2) return 19;

  /* Complex values pass and return by value, and element types convert. */
  cfloat rounded = unchanged(from_call);
  if (__imag__ rounded != 2) return 20;
  fcomplex narrowed = sum;
  if (__real__ narrowed != 1 || __imag__ narrowed != 90) return 21;
  icomplex truncated = narrowed;
  if (__real__ truncated != 1 || __imag__ truncated != 90) return 22;

  /* A complex value is not a scalar: int still prefers double, and the
     complex conversion is what makes a complex-only overload viable. */
  if (overloaded(1) != 2) return 23;
  icomplex one = 1;
  if (overloaded(one) != 1) return 24;
  if (!(!0i)) return 25;
  icomplex selected_real = one ? one : 0;
  if (__real__ selected_real != 1 || __imag__ selected_real != 0) return 26;
  icomplex selected_imaginary = one ? 0i : one;
  if (__real__ selected_imaginary != 0 || __imag__ selected_imaginary != 0)
    return 27;

  /* Integer element types keep their own rank and signedness. */
  scomplex shorts = 1 + 2i;
  scomplex short_sum = shorts + shorts;
  if (__real__ short_sum != 2 || __imag__ short_sum != 4) return 31;
  ccomplex chars = 1 + 2i;
  ccomplex char_sum = chars + chars;
  if (__real__ char_sum != 2 || __imag__ char_sum != 4) return 32;
  ucomplex unsigneds = 1u + 2i;
  ucomplex unsigned_sum = unsigneds + unsigneds;
  if (__real__ unsigned_sum != 2 || __imag__ unsigned_sum != 4) return 33;

  exit(0);
}
