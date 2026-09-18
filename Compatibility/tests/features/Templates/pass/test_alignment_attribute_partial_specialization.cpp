/* A declaration-specifier attribute belongs to the class-head, so a partial
   specialization may spell it between the class-key and the name:
   `struct __declspec(align(1)) S<Len, 1>`, `struct alignas(8) S<Len, 2>` and
   `struct [[nodiscard]] S<Len, 3>`.  The scan that decides whether a class
   declaration opens a partial specialization required the class-key to be the
   token immediately before the name, so such a declaration was classified as
   the primary template again and collided with it ("class template
   redeclaration has different parameters").  boost/move/detail/type_traits.hpp:982
   declares its aligned partial specializations exactly that way, which stopped
   the Explorer++ consumer build.  The attribute must end neither the class-key
   context nor the specialization's identity, and alignas still reaches the
   instantiated layout. */

template<unsigned long Len, unsigned long Align>
struct aligned_struct;

template<unsigned long Len>
struct __declspec(align(1)) aligned_struct<Len, 1>
{
    unsigned char data[Len];
};

template<unsigned long Len>
struct alignas(8) aligned_struct<Len, 2>
{
    unsigned char data[Len];
};

template<unsigned long Len>
struct [[nodiscard]] aligned_struct<Len, 3>
{
    unsigned char data[Len];
};

int main()
{
    aligned_struct<1, 1> fixed_one;
    aligned_struct<1, 2> fixed_two;
    aligned_struct<1, 3> standard_attribute;
    return sizeof(fixed_one) == 1 && sizeof(fixed_two) == 8
           && alignof(fixed_two) == 8 && sizeof(standard_attribute) == 1 ? 0 : 1;
}
