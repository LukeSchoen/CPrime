#ifndef TEST_CLSTRING_H
#define TEST_CLSTRING_H

#include <stddef.h>

class clString
{
public:
  clString(const char *text);

  int Length();
  void SetText(const char *text);
  char at(int index);
  clString operator+(clString rhs);

public:
  char *m_data;
  int m_size;
};

#endif
