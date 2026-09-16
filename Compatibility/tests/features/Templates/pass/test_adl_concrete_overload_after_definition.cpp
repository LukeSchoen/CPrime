// EXPECT_EXIT: 0
// An unqualified call in a template body also reaches the overloads the
// argument types associate at the point of instantiation, so a concrete
// overload declared after the template definition still owns the call and the
// generic template's body is never instantiated for it.
class clImage
{
public:
  clImage() : m_value(0) {}
  int m_value;
};

class clImageStream
{
};

template<typename T>
long clStreamRead(T *pTarget, long count, clImageStream *pStream)
{
  static_assert(false, "clStreamRead is not defined for this type.");
}

template<typename T>
T ReadValue(clImageStream *pStream)
{
  char buffer[sizeof(T)];
  clStreamRead((T *)buffer, 1, pStream);
  return *(T *)buffer;
}

long clStreamRead(bool *pTarget, long count, clImageStream *pStream);

long clStreamRead(clImage *pTarget, long count, clImageStream *pStream)
{
  (void)count;
  (void)pStream;
  pTarget->m_value = 7;
  return 1;
}

int main()
{
  clImageStream stream;
  clImage image = ReadValue<clImage>(&stream);
  return image.m_value == 7 ? 0 : 1;
}
