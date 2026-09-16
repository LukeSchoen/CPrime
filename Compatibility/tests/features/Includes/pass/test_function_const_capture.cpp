#include <functional>
long long call(const std::function<long long()>& f){return f();}
long long run(const std::function<long long()>& payload){auto wrapper=[=]()->long long{return payload();};return call(wrapper);}
int main(){return run([]()->long long{return 7;})!=7;}
