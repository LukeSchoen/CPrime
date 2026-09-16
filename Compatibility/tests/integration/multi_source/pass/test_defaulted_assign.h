#ifndef TEST_DEFAULTED_ASSIGN_H
#define TEST_DEFAULTED_ASSIGN_H

struct Box
{
  int value;
  Box() = default;
  Box &operator=(const Box &rhs) = default;
};

int assign_value(Box *dst, const Box *src);

#endif
