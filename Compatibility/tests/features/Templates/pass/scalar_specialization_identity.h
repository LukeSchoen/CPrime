#include <stddef.h>

typedef unsigned char Byte;
typedef unsigned long UnsignedLong;
typedef unsigned int UnsignedInt;
typedef unsigned short UnsignedShort;
enum class EnumValue : unsigned int { item = 0 };

template<class T> struct Identity;
template<> struct Identity<Byte> { static int value() { return 7; } };
template<> struct Identity<signed char> { static int value() { return 8; } };
template<> struct Identity<char> { static int value() { return 9; } };
template<> struct Identity<UnsignedLong> { static int value() { return 10; } };
template<> struct Identity<UnsignedInt> { static int value() { return 11; } };
template<> struct Identity<UnsignedShort> { static int value() { return 12; } };
template<> struct Identity<wchar_t> { static int value() { return 13; } };
template<> struct Identity<EnumValue> { static int value() { return 14; } };

template<class T> struct Strip { typedef T type; };
template<class T> int through_alias() { return Identity<typename Strip<T>::type>::value(); }
template<class T> int through_deduction(T) { return through_alias<T>(); }

static int check_scalar_specializations()
{
    if (Identity<unsigned char>::value() != 7 || through_alias<Byte>() != 7) return 1;
    if (through_alias<signed char>() != 8 || through_alias<char>() != 9) return 2;
    if (through_alias<unsigned long>() != 10 || through_alias<unsigned int>() != 11) return 3;
    if (through_alias<unsigned short>() != 12 || through_alias<wchar_t>() != 13) return 4;
    if (through_alias<EnumValue>() != 14) return 5;
    Byte byte = 0;
    char plain = 0;
    signed char signed_byte = 0;
    UnsignedLong number = 0;
    wchar_t wide = 0;
    if (through_deduction(byte) != 7 || through_deduction(plain) != 9) return 6;
    if (through_deduction(signed_byte) != 8 || through_deduction(number) != 10) return 7;
    if (through_deduction(wide) != 13 || through_deduction(EnumValue::item) != 14) return 8;
    return 0;
}
