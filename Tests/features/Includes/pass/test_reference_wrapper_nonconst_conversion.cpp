#include <functional>
struct Object {
 int value;Object():value(3){}
 template<class T> Object(const T& t):value(t.invalid_conversion()){}
};
void update(Object& value){value.value=7;}
int main(){Object value;update(std::ref(value));return value.value!=7;}
