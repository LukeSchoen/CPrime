typedef long long i64;

class Incomplete;

template<typename T>
class Box
{
public:
  i64 Size() const;
  void UnusedBad();
};

template<typename T>
i64 Box<T>::Size() const
{
  return 0;
}

template<typename T>
void Box<T>::UnusedBad()
{
  i64 value = 0;
  value.Empty();
}

class Owner
{
public:
  static Incomplete Use(const Box<Incomplete> &items);
};

class Incomplete
{
public:
  int value;
};

int main()
{
  return 0;
}
