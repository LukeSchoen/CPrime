struct HookBase
{
  int value;
};

typedef struct HookDerived : public HookBase
{
  int extra;
} HookDerived, *HookDerivedPtr;

int main()
{
  HookDerived hook;
  HookDerivedPtr pointer = &hook;
  hook.value = 17;
  hook.extra = 25;
  return pointer->value + pointer->extra == 42 ? 0 : 1;
}
