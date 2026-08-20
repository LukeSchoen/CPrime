#ifndef TEST_OUT_OF_CLASS_TEMPLATE_MEMBER_H
#define TEST_OUT_OF_CLASS_TEMPLATE_MEMBER_H

template<typename T>
class OutOfClassBox
{
public:
  T value;

  void Set(const T &next);
  T Get() const;
  void Clear();
};

#include "test_out_of_class_template_member.inl"

#endif
