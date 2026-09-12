#ifndef CPRIME_TEST_CLASS_TEMPLATE_MEMBER_LINK_H
#define CPRIME_TEST_CLASS_TEMPLATE_MEMBER_LINK_H

template <typename T>
struct LinkedMembers
{
  T values[4];

  const T &Get(int index) const;
  T Sum() const;
};

template <typename T>
const T &LinkedMembers<T>::Get(int index) const
{
  return values[index];
}

template <typename T>
T LinkedMembers<T>::Sum() const
{
  return Get(0) + Get(1);
}

#endif
