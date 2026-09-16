#ifndef TEST_HEADER_TEMPLATE_INLINE_MEMBER_H
#define TEST_HEADER_TEMPLATE_INLINE_MEMBER_H

template<typename T>
class HeaderTemplateThing
{
public:
  T value;

  T get(void)
  {
    return this->value;
  }
};

#endif
