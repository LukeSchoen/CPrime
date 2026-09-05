template<class T> struct Result {
    T value;
    bool put(T input) { value = input; return true; }
};
struct Options { int flags = 0; };
template<class T> struct Index {
    template<class R>
    bool find(R &result, const T *values, const Options &options = {}) const {
        return result.put(*values);
    }
};
template<class T> struct Adapter { Index<T> *index; };
int main() {
    Index<float> index;
    Adapter<float> adapter;
    adapter.index = &index;
    void *pointer = &adapter;
    Result<float> result;
    float value = 7;
    ((Adapter<float> *)pointer)->index->find(result, &value);
    return result.value != 7;
}
