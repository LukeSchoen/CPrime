// EXPECT_EXIT: 0
typedef unsigned int Word;
typedef long long Count;

template<class T> struct Buffer {
    Buffer();
    Buffer(const Buffer& other);
    Buffer(Buffer&& other);
    Buffer(const Count& size, const T& value);
    ~Buffer();
    void reserve(Count size, bool force = false);
    bool try_reserve(Count size, bool force = false);
    Count count;
};

template<class T> Buffer<T>::Buffer(Buffer&& other) : Buffer<T>() {
    count = other.count;
}
template<class T> Buffer<T>::Buffer(const Buffer& other) : Buffer<T>() {
    reserve(other.count);
}
template<class T> Buffer<T>::Buffer() : count(0) {}
template<class T> Buffer<T>::Buffer(const Count& size, const T& value)
    : Buffer<T>() { reserve(size); }
template<class T> Buffer<T>::~Buffer() {}
template<class T> void Buffer<T>::reserve(Count size, bool force) {
    try_reserve(size, force);
}
template<class T> bool Buffer<T>::try_reserve(Count size, bool force) {
    count = size;
    return true;
}

// Completing the derived base requires Buffer's layout during another
// template replay, before the first call selects any Buffer constructor.
template<class T> struct Array { Buffer<T> values; };
struct Image : Array<Word> {};

int main() {
    Buffer<Word> first(7, 1u);
    Buffer<unsigned int> second(first);
    if (second.count != 7) return 1;
    Image image;
    image.values.reserve(13);
    Image copy(image);
    return copy.values.count != 13;
}
