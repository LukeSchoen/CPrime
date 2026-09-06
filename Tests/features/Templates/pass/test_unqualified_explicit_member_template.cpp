struct Loader {
 void* value;
 template<class F> F* get(){return cast<F>();}
 template<class T> T* cast(){return static_cast<T*>(value);}
};int main(){int n=7;Loader l={&n};return l.get<int>()!=&n;}
