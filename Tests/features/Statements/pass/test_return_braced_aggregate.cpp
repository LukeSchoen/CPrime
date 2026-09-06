#include <initializer_list>
struct Controls { float yaw, pitch, roll, thrust, brake; bool boost; };
Controls controls(float first, bool flag) { return {first,2,3,4,5,flag}; }
struct Nested { int values[3]; Controls controls; };
Nested nested() { return {{6,7,8},{1,2,3,4,5,true}}; }
struct List {
 int count, sum;
 List(std::initializer_list<int> values):count(0),sum(0) {
  for (const int* p=values.begin(); p!=values.end(); ++p) { ++count; sum+=*p; }
 }
};
List list() { return {1,2,3}; }
int main() {
 Controls c=controls(1.5f,true);
 if (c.yaw!=1.5f || c.pitch!=2 || c.roll!=3 || c.thrust!=4 || c.brake!=5 || !c.boost) return 1;
 Nested n=nested();
 if(n.values[0]!=6 || n.values[1]!=7 || n.values[2]!=8 || !n.controls.boost) return 2;
 List values=list();
 return values.count!=3 || values.sum!=6;
}
