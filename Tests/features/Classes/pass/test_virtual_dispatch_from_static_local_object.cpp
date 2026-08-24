class StaticReadBase
{
public:
  int Read()
  {
    return StreamRead();
  }

protected:
  virtual int StreamRead()
  {
    return 1;
  }
};

class StaticFileReader : public StaticReadBase
{
protected:
  virtual int StreamRead() override
  {
    return 2;
  }
};

int read_static_file()
{
  static StaticFileReader reader;
  return reader.Read();
}

int main()
{
  return read_static_file() == 2 && read_static_file() == 2 ? 0 : 1;
}
