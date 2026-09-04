#include <crtdbg.h>

int main()
{
  _CrtMemState state = {};
  return state.lTotalCount != 0;
}
