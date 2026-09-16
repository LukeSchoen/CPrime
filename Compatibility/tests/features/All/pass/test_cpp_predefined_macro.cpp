#ifndef __cplusplus
#error __cplusplus must be defined when compiling C++
#endif

#if __cplusplus < 201402L
#error CPrime advertises C++14 compatibility or later
#endif

int main()
{
  return 0;
}
