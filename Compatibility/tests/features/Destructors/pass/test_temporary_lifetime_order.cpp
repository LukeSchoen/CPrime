// EXPECT_EXIT: 0
int logv;

struct T
{
  int v;
  T(int x);
  T operator+(T rhs);
  ~T();
};

T::T(int x)
{
  this->v = x;
  logv = logv * 10 + x;
}

T T::operator+(T rhs)
{
  T out(this->v + rhs.v);
  return out;
}

T::~T()
{
  logv = logv * 10 + this->v;
}

void take(T a, T b)
{
  logv = logv * 10 + 9;
}

int main(void)
{
  take(T(1) + T(2), T(3));
  return logv != 0 ? 0 : 1;
}

