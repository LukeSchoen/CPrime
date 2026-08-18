#include <string.h>

class Buffer
{
public:
    void Clear();
    unsigned char *data;
};

void Buffer::Clear()
{
    memset(data, 7, 1);
}

int main()
{
    unsigned char value = 0;
    Buffer buffer;
    buffer.data = &value;
    buffer.Clear();
    return value == 7 ? 0 : 1;
}
