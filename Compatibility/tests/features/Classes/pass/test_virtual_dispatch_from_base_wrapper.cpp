class ReadBase
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

class FileReader : public ReadBase
{
protected:
  virtual int StreamRead() override
  {
    return 2;
  }
};

int main()
{
  FileReader reader;
  return reader.Read() == 2 ? 0 : 1;
}
