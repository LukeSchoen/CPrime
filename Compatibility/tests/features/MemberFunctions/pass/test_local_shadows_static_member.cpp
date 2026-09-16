struct Base {static const int count=90;};
struct Object:Base {int run();static int run_static();};
int Object::run(){int result=count;{auto count=7;result+=count;}return result+count;}
int Object::run_static(){int result=count;{auto count=4;result+=count;}return result+count;}
int main(){Object o;return o.run()!=187||Object::run_static()!=184;}
