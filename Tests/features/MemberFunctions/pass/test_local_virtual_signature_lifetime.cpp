// EXPECT_COMPILE_ARGS: -Werror
struct First { int value; };
struct Second { int value; };
struct Third { int value; };
struct Fourth { int value; };
struct Visitor {
 virtual int Visit(const First&) { return 10; }
 virtual int Visit(const Second&) { return 20; }
 virtual int Visit(const Third&) { return 30; }
 virtual int Visit(const Fourth&) { return 40; }
 virtual ~Visitor() {}
};
template<class T,int N> struct Buffer {
 T local[N]; T* values; int count; int capacity;
 Buffer():values(local),count(0),capacity(N) {}
 ~Buffer() { if(values!=local) delete[] values; }
 void Ensure(int n) {
  if(n<=capacity) return;
  T* replacement=new T[n*2];
  for(int i=0;i<count;++i) replacement[i]=values[i];
  if(values!=local) delete[] values;
  values=replacement; capacity=n*2;
 }
 void Push(T value) { Ensure(count+1); values[count++]=value; }
 T* Data() { return values; }
};
int dispatch(Visitor& visitor) {
 First a={1}; Second b={2}; Third c={3}; Fourth d={4};
 return visitor.Visit(a)*1000+visitor.Visit(b)*100+visitor.Visit(c)*10+visitor.Visit(d);
}
int main() {
 struct Local : Visitor {
  Buffer<char,16> buffer;
  int Visit(const First& first) { buffer.Push('a'); return first.value; }
  int Visit(const Second& second) { buffer.Push('b'); return second.value; }
  int Visit(const Third& third) { buffer.Push('c'); return third.value; }
  int Visit(const Fourth& fourth) { buffer.Push('d'); return fourth.value; }
  bool valid() { char* p=buffer.Data(); return p[0]=='a'&&p[1]=='b'&&p[2]=='c'&&p[3]=='d'; }
 } local;
 if(dispatch(local)!=1234) return 1;
 return !local.valid();
}
