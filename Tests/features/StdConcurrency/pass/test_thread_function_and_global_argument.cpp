#include <thread>
int calls;void work(int* p,int n){*p=n;}
int main(){std::thread task(work,&calls,7);task.join();return calls!=7;}
