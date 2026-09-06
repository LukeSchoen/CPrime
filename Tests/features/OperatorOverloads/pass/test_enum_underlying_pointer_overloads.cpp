enum Kind:unsigned char {One=1};
int read(const unsigned char*){return 3;}
int read(const Kind*){return 7;}
template<class T>int read(const T*){static_assert(sizeof(T)==0,"fallback selected");return 0;}
template<class T>int dispatch(const T* p){return read(p);}
int main(){unsigned char byte=1;Kind kind=One;return dispatch(&byte)!=3||dispatch(&kind)!=7;}
