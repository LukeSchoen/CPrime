#pragma once
namespace special_link {
struct Value {
  int bias;
  template<class T> int get(T value) const { return bias + 1; }
};
template<class T> int calculate(T value) { return 2; }
template<> int Value::get(int value) const;
template<> int calculate(int value);
}
