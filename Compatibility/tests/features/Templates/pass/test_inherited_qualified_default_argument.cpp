namespace ns {
struct Mode {typedef int flags;static const flags input=1;};
template<class T>struct Base:Mode{};
template<class T>struct Stream:Base<T> {int value;explicit Stream(const char* path,Mode::flags mode=Mode::input):value(mode){} };
}
int main(){ns::Stream<char> input("file");return input.value!=1;}
