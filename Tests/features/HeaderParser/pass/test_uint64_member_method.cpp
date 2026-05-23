#include <string.h>

typedef unsigned long long Uint64;

class InputMessage
{
public:
  unsigned char *m_networkBuffer;
  unsigned int m_readPos;

  Uint64 getU64()
  {
    if (!m_networkBuffer)
      return 0;

    Uint64 v;
    memcpy(&v, m_networkBuffer + m_readPos, 8);
    m_readPos += 8;
    return v;
  }
};

int main(void)
{
  return 0;
}
