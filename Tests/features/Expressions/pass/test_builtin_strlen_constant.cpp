// EXPECT_EXIT: 0
namespace std {
typedef __SIZE_TYPE__ size_t;
extern "C" size_t strlen(const char *);
}
using namespace std;

static int bad;
extern "C" void link_error(void) { bad = 1; }

int main() {
  if (strlen("foo") != 3)
    link_error();
  if (std::strlen("bar") != 3)
    link_error();
  return bad;
}
