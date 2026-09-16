#ifndef LIST_H
#define LIST_H

template<typename T>
class List
{
public:
  T *items;
  int count;
  int capacity;

  List();
  ~List();
  void push(T value);
  T get(int index);
  int size();
};

#endif
