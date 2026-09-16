typedef unsigned long long size_t;
#include <initializer_list>

template<typename T>
struct RefOverloadBox {
    int value;

    RefOverloadBox()
      : value(0)
    {
    }

    RefOverloadBox(const RefOverloadBox<T> &other);
    RefOverloadBox(RefOverloadBox<T> &&other);
    void PushFront(T &&value);
    void PushFront(const T &value);
    void PushFront(const RefOverloadBox<T> &values);
    void PushFront(RefOverloadBox<T> &&values);
    void PushFront(const std::initializer_list<T> &values);
};

template<typename T>
RefOverloadBox<T>::RefOverloadBox(const RefOverloadBox<T> &other)
  : value(1)
{
}

template<typename T>
RefOverloadBox<T>::RefOverloadBox(RefOverloadBox<T> &&other)
  : value(2)
{
}

template<typename T>
void RefOverloadBox<T>::PushFront(T &&value)
{
    this->value = 1;
}

template<typename T>
void RefOverloadBox<T>::PushFront(const T &value)
{
    this->value = 2;
}

template<typename T>
void RefOverloadBox<T>::PushFront(const RefOverloadBox<T> &values)
{
    this->value = 3;
}

template<typename T>
void RefOverloadBox<T>::PushFront(RefOverloadBox<T> &&values)
{
    this->value = 4;
}

template<typename T>
void RefOverloadBox<T>::PushFront(const std::initializer_list<T> &values)
{
    this->value = 5;
}

typedef RefOverloadBox<int> IntRefOverloadBox;

int main()
{
    return 0;
}
