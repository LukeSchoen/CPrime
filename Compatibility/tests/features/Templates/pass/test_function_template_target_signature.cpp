template<class T> T identity(T value){return value;}
int apply(int(*function)(int)){return function(7);}
int main(){int(*p)(int)=identity;return p(5)==5 && apply(identity)==7?0:1;}
