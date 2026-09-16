struct Owner {struct Nested {using Type=int;};};
int read(Owner::Nested::Type const& x){return x;}
int main(){int n=7;return read(n)==7?0:1;}
