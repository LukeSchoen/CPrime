#ifndef TEST_TEXT_BLOCK_H
#define TEST_TEXT_BLOCK_H

#include <stddef.h>

class TextBlock
{
public:
  TextBlock(const char *text);

  int Length();
  void SetText(const char *text);
  char at(int index);
  TextBlock operator+(TextBlock rhs);

public:
  char *m_data;
  int m_size;
};

#endif
