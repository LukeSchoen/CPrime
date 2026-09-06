#include <string>
namespace library {namespace detail {namespace {
struct Writer{typedef unsigned short* value_type;};
template<int N>struct Select;
template<>struct Select<2>{typedef Writer writer;};
typedef Select<sizeof(wchar_t)>::writer WideWriter;
}}}
namespace library {namespace detail {namespace {
int convert(){std::wstring result;result.resize(4);WideWriter::value_type begin=reinterpret_cast<WideWriter::value_type>(&result[0]);*begin=65;return result[0];}
}}}
int main(){return library::detail::convert()!=65;}
