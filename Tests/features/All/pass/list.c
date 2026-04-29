#include "list.h"

template <typename T>
List<T>::List()
{
  this->items = (T *)malloc(sizeof(T) * 8);
  this->count = 0;
  this->capacity = 8;
}

template<typename T>
List<T>::~List()
{
  free(this->items);
  this->items = 0;
  this->count = 0;
  this->capacity = 0;
}

template<typename T>
void List<T>::push(T value)
{
  if (this->count == this->capacity)
  {
    this->capacity = this->capacity * 2;
    this->items = (T *)realloc(this->items, sizeof(T) * this->capacity);
  }
  this->items[this->count] = value;
  this->count = this->count + 1;
}

template<typename T>
T List<T>::get(int index)
{
  return this->items[index];
}

template<typename T>
int List<T>::size()
{
  return this->count;
}
