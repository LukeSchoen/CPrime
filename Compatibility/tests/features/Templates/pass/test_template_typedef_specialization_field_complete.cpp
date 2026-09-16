// EXPECT_EXIT: 0

typedef unsigned char ui8;
typedef signed long long int i64;

class Stream;

template<typename T>
class List;

template<typename T>
i64 StreamWrite(const List<T> *pValues, i64 count, Stream *pStream);

template<typename T>
i64 StreamRead(List<T> *pValues, i64 count, Stream *pStream);

template<typename T>
class List
{
public:
  List()
  {
    m_size = 0;
    m_data = 0;
  }

  i64 Size() const
  {
    return m_size;
  }

  template<typename C>
  friend i64 StreamWrite(const List<C> *pValues, i64 count, Stream *pStream);

  template<typename C>
  friend i64 StreamRead(List<C> *pValues, i64 count, Stream *pStream);

private:
  i64 m_size;
  T *m_data;
};

typedef List<ui8> ByteList;

class Writer
{
private:
  ByteList m_data;

public:
  i64 Length() const
  {
    return m_data.Size();
  }
};

int main()
{
  Writer writer;
  return writer.Length() == 0 ? 0 : 1;
}
