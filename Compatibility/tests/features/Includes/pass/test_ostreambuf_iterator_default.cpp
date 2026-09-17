#include <iterator>
#include <sstream>
#include <string>

template <class CharT,
          class OutItrT = std::ostreambuf_iterator<CharT,
                                                    std::char_traits<CharT> > >
class period_formatter
{
public:
  typedef std::basic_string<CharT> string_type;
  typedef typename string_type::const_iterator const_itr_type;
  OutItrT put_separator(OutItrT &oitr, const string_type &separator) const
  {
    const_itr_type ci = separator.begin();
    while (ci != separator.end())
    {
      *oitr = *ci;
      ++ci;
    }
    return oitr;
  }
};

int main()
{
  period_formatter<char> formatter;
  std::ostringstream out;
  std::ostreambuf_iterator<char> oitr(out.rdbuf());
  formatter.put_separator(oitr, std::string("/"));
  if (out.str() != "/")
    return 1;
  if (std::ostreambuf_iterator<char>(out).failed())
    return 2;
  return 0;
}
