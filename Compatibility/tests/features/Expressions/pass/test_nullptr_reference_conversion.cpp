#include <utility>
struct Item {};
int check(Item* const& p){return p==nullptr;}
template<class T>int forward(T&& value){return check(std::forward<T>(value));}
int number(){return 7;}
int function_reference(int(*const& fn)()){return fn();}
int main(){decltype(nullptr) value=nullptr;return !forward(nullptr)||!forward(value)||function_reference(number)!=7;}
