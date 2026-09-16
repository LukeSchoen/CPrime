#include <functional>
struct Base { std::function<int(int)> callback; };
struct Object:Base { int run(int); };
int Object::run(int n){return callback(n);}
int main(){Object o;o.callback=[](int n){return n+7;};return o.run(3)!=10;}
