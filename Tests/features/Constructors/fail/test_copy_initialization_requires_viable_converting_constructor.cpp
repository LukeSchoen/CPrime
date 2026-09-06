// EXPECT_COMPILE_FAIL: 1
template<class T> struct Box {
 T value;
 Box() : value(7) {}
 template<class U> Box(const Box<U>& other,int required) : value(other.value+required) {}
};
int main(){Box<int> source;Box<double> target=source;return target.value!=7;}
