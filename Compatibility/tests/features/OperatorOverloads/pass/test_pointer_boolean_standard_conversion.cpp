// EXPECT_COMPILE_ARGS: -Werror
int choose(bool value) { return value ? 1 : 2; }
int choose(int) { return 3; }
int choose(double) { return 4; }
int address(void*) { return 5; }
int address(bool) { return 6; }
int reference(const bool &value) { return value ? 7 : 8; }
int reference(int) { return 9; }
int rvalue(bool &&value) { return value ? 10 : 11; }
int rvalue(double) { return 12; }
void function() {}
struct Member { static int choose(bool v) { return v ? 13 : 14; } static int choose(int) { return 15; } };
template<class T> int forward(T value) { return choose(value); }
int main() {
 int value=1; int *pointer=&value; int *null_pointer=0; int *const fixed=pointer;
 char array[4]={};
 if(choose(pointer)!=1 || choose(null_pointer)!=2 || choose(static_cast<char*>(array))!=1) return 1;
 if(choose(&function)!=1 || forward(pointer)!=1 || Member::choose(pointer)!=13) return 2;
 if(address(pointer)!=5 || reference(pointer)!=7 || reference(null_pointer)!=8) return 3;
 if(rvalue(pointer)!=10 || rvalue(null_pointer)!=11 || reference(fixed)!=7) return 4;
 return 0;
}
