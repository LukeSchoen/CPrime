// A std::basic_ios instantiation publishes the member types the standard gives
// it, and std::basic_ostream reaches them through its base.  boost/log's
// formatting_ostream.hpp names ostream_type::int_type and ostream_type::failure
// through a template parameter, so both spellings have to be types.
#include <ostream>

typedef std::basic_ios<char>::int_type ios_int_type;
typedef std::basic_ostream<char>::int_type stream_int_type;
typedef std::basic_ostream<char>::char_type stream_char_type;

int main()
{
  ios_int_type eof_value = std::char_traits<char>::eof();
  stream_int_type same_value = std::char_traits<char>::eof();
  stream_char_type letter = 'x';
  if (!std::char_traits<char>::eq_int_type(eof_value, same_value))
    return 1;
  if (letter != 'x')
    return 2;
  return 0;
}
