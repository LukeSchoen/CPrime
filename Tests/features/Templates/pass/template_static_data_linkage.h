#pragma once
template<class T>struct SharedStore{static int value;};
template<class T>int SharedStore<T>::value=5;
int* other_store();
