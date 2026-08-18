struct MemberConstRefVec
{
  int value;
};

class MemberConstRefUser
{
public:
  const MemberConstRefVec &Get() const;
  void Use(const MemberConstRefVec &value);
};

const MemberConstRefVec &MemberConstRefUser::Get() const
{
  static MemberConstRefVec value = { 1 };
  return value;
}

void MemberConstRefUser::Use(const MemberConstRefVec &value)
{
  (void)value;
}

int main()
{
  MemberConstRefUser value;
  value.Use(value.Get());
  return 0;
}
