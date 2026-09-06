struct Path{int value;Path(int n):value(n){}};
struct File {
  template<class T> static int write(const Path&,T* source,int count);
  template<class T> static int write(const Path&,const T& source);
};
template<class T> int File::write(const Path& path,T* source,int count){return path.value+*source+count;}
template<class T> int File::write(const Path& path,const T& source){return write(path,&source,1);}
int earlier(int n){return File::write(3,&n,1);}
struct Caller { static int run(); };
int Caller::run(){int n=7;return File::write(3,n);}
int main(){return earlier(7)!=11 || Caller::run()!=11;}
