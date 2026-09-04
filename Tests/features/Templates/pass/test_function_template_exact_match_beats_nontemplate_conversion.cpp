namespace compatibility
{
enum Error
{
  OK = 0
};
}

using namespace compatibility;

int Pick(const char* name, Error left, Error right,
         bool echo = true, bool extra = false)
{
  return name[0] && left == right && echo && !extra ? 1 : 0;
}

int Pick(const char* name, bool left, bool right,
         bool echo = true, bool extra = false)
{
  return name[0] && left == right && echo && !extra ? 1 : 0;
}

template<class T>
int Pick(const char* name, T left, T right, bool echo = true)
{
  return name[0] && echo ? left + right : 0;
}

int main()
{
  return Pick("exact", 0, 3) == 3
             && Pick("nontemplate", true, false) == 0
           ? 0 : 1;
}
