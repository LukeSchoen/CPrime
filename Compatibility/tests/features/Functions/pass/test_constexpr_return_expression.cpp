constexpr long long huge() { return 9223372036854775807LL; }
constexpr long long nested() { return huge() - 9; }
namespace Values { constexpr int value=17; constexpr int read(){return value+2;} }
struct Constants { static const long long maximum = nested(); };
static_assert(Constants::maximum == 9223372036854775798LL,"wide static member");
int main(){
 int value=99;
 static_assert(Values::read()==19,"lexical lookup");
 return Constants::maximum==9223372036854775798LL && value==99?0:1;
}
