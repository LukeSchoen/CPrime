#include <functional>
struct Delegate { bool cancelled; };
long long invoke(const std::function<long long(const Delegate&)>& callback) {
  Delegate d={false};return callback(d);
}
int main(){int n=9;auto lambda=[=](const Delegate& d)->long long{return d.cancelled?0:n;};return invoke(lambda)!=9;}
