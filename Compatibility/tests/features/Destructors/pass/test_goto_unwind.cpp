// EXPECT_EXIT: 0
int g;

struct ScopeObj
{
  int v;

  ~ScopeObj();
};

ScopeObj::~ScopeObj()
{
  g += this->v;
}

int main(void)
{
  {
    struct ScopeObj o = {9};
    goto out;
  }
out:
  return g == 9 ? 0 : 1;
}
