template<typename T>
T &&ReplayMove(T &value)
{
  return value;
}

typedef struct
{
  int value;
} AnonymousReplayValue;

template<typename T>
int ReadMovedValue(T &&value)
{
  return ReplayMove(value).value;
}

int main()
{
  return ReadMovedValue(AnonymousReplayValue{17}) == 17 ? 0 : 1;
}

// EXPECT_EXIT: 0
