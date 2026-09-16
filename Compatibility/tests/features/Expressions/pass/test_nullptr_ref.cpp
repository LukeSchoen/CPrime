int use(const decltype(nullptr)& n){return n!=nullptr;} int main(){auto n=nullptr;return use(n)||use(nullptr);}
