struct Writer
{
  template<class T>
  int Write(const T *values, int count)
  {
    return count == 1 ? *values : 0;
  }
};

int main()
{
  Writer writer;
  int value = 31;
  return writer.Write(&value, 1) == 31 ? 0 : 1;
}
