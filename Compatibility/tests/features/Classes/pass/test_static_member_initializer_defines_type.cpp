// EXPECT_EXIT: 0
/* A new type may be created inside the initializer of an out-of-class static
   data member definition.  The definition is copied into the member's class
   scope before it is parsed, and that copy used to end at the first `;`,
   which is the one inside the new type's own body, so the initializer was
   truncated and the definition failed with `unexpected end of file`.  The
   type belongs to the initializer's own expression: it must not become a
   member of the class being defined. */

struct Word
{
    unsigned char bytes[8];
    double value;
};

class DoubleSupport
{
public:
    static void toDouble();

    static const double s_positiveInfinity;
    static const int word_size;
    static const int nested_braces;
};

/* The row's shape: an obsolete designated initializer inside a union compound
   literal whose body holds the `;` that ended the copy. */
const double DoubleSupport::s_positiveInfinity =
(__extension__ ((union { unsigned char __c[8]; double __d; })
  { __c: { 0, 0, 0, 0, 0, 0, 0xf0, 0x7f } }).__d);

const int DoubleSupport::word_size = sizeof(struct { unsigned char bytes[8]; double value; });

/* A brace inside a lambda body is the same shape without any type name. */
const int DoubleSupport::nested_braces = [] { return (int)sizeof(Word); }();

struct other
{
};

void
DoubleSupport::toDouble()
{
}

int main()
{
    if (!(DoubleSupport::s_positiveInfinity > 1.7e308))
        return 1;
    if (DoubleSupport::word_size != (int)sizeof(Word))
        return 2;
    if (DoubleSupport::nested_braces != (int)sizeof(Word))
        return 3;
    /* Only the declared static members exist: the initializer's new type was
       not injected into this class's scope. */
    if (sizeof(DoubleSupport) != 1)
        return 4;
    return 0;
}
