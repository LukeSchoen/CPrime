typedef long long i64;
typedef unsigned char ui8;

class ReadStream;
class WriteStream;

template<typename T>
struct AliasFriendList {
    using iterator = T *;
    using const_iterator = const T *;
    using Element = T;

    AliasFriendList();

    template<typename C>
    friend i64 streamWrite(const AliasFriendList<C> *values, i64 count, WriteStream *stream);

    template<typename C>
    friend i64 streamRead(AliasFriendList<C> *dst, i64 count, ReadStream *stream);

    T value;
};

template<typename T>
AliasFriendList<T>::AliasFriendList()
  : value(0)
{
}

typedef AliasFriendList<ui8> ByteFriendList;

int main()
{
    ByteFriendList list;
    list.value = 4;
    return list.value == 4 ? 0 : 1;
}
