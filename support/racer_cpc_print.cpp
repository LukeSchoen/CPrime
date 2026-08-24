#include "clPrint.h"

i64 clPrint::Int(char *text, i64 value)
{
  char *cursor = text;
  if (value == 0)
  {
    *cursor = '0';
    return 1;
  }
  if (value < 0)
  {
    *cursor++ = '-';
    value = -value;
  }
  char *digits = cursor;
  while (value > 0)
  {
    i64 next = value / 10;
    *digits++ = '0' + char(value - next * 10);
    value = next;
  }
  i64 length = digits - text;
  while (cursor < digits)
  {
    char temp = *cursor;
    *cursor++ = *--digits;
    *digits = temp;
  }
  return length;
}

clString clPrint::Int(i64 value)
{
  char text[32];
  i64 length = Int(text, value);
  return clString(text, length);
}
