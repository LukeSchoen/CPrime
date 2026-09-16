#include <functional>
int called=0;
void use(const std::function<void()>& f){f();}
int main(){use([]()->int{++called;return 7;});return called!=1;}
