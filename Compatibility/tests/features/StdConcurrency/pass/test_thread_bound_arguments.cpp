#include <thread>
#include <functional>
static int result;
struct Worker {void run(int n){result=n;}};
void update(int&target,int first,int second){target=first+second;}
int main(){Worker w;std::thread a(&Worker::run,&w,17);a.join();if(result!=17)return 1;std::thread b(update,std::ref(result),9,12);b.join();return result!=21;}
