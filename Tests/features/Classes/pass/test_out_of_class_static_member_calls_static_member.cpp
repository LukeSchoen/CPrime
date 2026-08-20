class StaticCallPath
{
public:
  int value;
};

class StaticCallFolder
{
public:
  static bool Exists(StaticCallPath path);
  static bool Create(StaticCallPath path);
};

class StaticCallFile
{
public:
  static bool Delete(StaticCallPath path);
};

bool StaticCallFolder::Exists(StaticCallPath path)
{
  return path.value != 0;
}

bool StaticCallFile::Delete(StaticCallPath path)
{
  return path.value == 2;
}

bool StaticCallFolder::Create(StaticCallPath path)
{
  if (StaticCallFolder::Exists(path))
    return true;
  if (StaticCallFile::Delete(path))
    return true;
  return false;
}

int main()
{
  StaticCallPath path;
  path.value = 2;
  return StaticCallFolder::Create(path) ? 0 : 1;
}
