template<class T> struct SharedList {
    T value;
    SharedList& operator=(const SharedList& other);
    static int identity(int value);
};
template<class T>
SharedList<T>& SharedList<T>::operator=(const SharedList<T>& other) {
    value = other.value; return *this;
}
template<class T> int SharedList<T>::identity(int value) { return value; }
