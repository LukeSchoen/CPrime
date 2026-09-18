// EXPECT_EXIT: 0
// An unqualified call in a template body also reaches the overloads the
// argument types associate at the point of instantiation, so a concrete
// overload declared after the template definition still owns the call and the
// generic template's body is never instantiated for it.
class Image
{
public:
  Image() : m_value(0) {}
  int m_value;
};

class ImageStream
{
};

template<typename T>
long streamRead(T *pTarget, long count, ImageStream *pStream)
{
  static_assert(false, "streamRead is not defined for this type.");
}

template<typename T>
T ReadValue(ImageStream *pStream)
{
  char buffer[sizeof(T)];
  streamRead((T *)buffer, 1, pStream);
  return *(T *)buffer;
}

long streamRead(bool *pTarget, long count, ImageStream *pStream);

long streamRead(Image *pTarget, long count, ImageStream *pStream)
{
  (void)count;
  (void)pStream;
  pTarget->m_value = 7;
  return 1;
}

int main()
{
  ImageStream stream;
  Image image = ReadValue<Image>(&stream);
  return image.m_value == 7 ? 0 : 1;
}
