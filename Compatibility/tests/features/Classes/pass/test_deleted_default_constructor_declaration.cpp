class DeletedDefaultCtor
{
public:
  DeletedDefaultCtor() = delete;

  static int value()
  {
    return 7;
  }
};

int main()
{
  return DeletedDefaultCtor::value() == 7 ? 0 : 1;
}
