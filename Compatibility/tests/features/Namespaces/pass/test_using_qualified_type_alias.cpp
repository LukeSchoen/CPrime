namespace outer { namespace inner { using Handle = void*; using Null = decltype(nullptr); } }
using outer::inner::Handle;
using outer::inner::Null;
int main() { Handle p = nullptr; Null n = nullptr; return p != n || sizeof(Null) != sizeof(void*); }
