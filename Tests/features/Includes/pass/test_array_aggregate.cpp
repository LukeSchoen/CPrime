#include <array>
#include <string>
const std::array<int,3>& values(){static const std::array<int,3> v={{1,2,3}};return v;}
int main(){std::array<std::string,2> a={{"first","second"}};auto b=a;b[0]='x';if(a[0]!="first"||b[0]!="x"||b[1]!="second"||values()[2]!=3)return 1;try{b.at(2);return 2;}catch(const std::out_of_range&){}return 0;}
