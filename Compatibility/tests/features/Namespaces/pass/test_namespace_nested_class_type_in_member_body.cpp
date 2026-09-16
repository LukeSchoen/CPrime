namespace compatibility
{
class Owner
{
public:
  enum Kind { READY = 3 };

  Kind GetKind() const
  {
    return READY;
  }

  class Guard
  {
  public:
    explicit Guard(int* value)
    {
      ++*value;
    }
  };
};

class Worker
{
public:
  int Run();
};

int Worker::Run()
{
  int value = 0;
  Owner::Guard guard(&value);
  return value;
}
}

int main()
{
  compatibility::Worker worker;
  compatibility::Owner owner;
  return worker.Run() == 1 && owner.GetKind() == compatibility::Owner::READY
           ? 0 : 1;
}
