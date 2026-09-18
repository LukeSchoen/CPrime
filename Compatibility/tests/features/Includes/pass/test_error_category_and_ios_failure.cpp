// EXPECT_COMPILE_ARGS: -std=c++17
// The error-category surface behind <system_error>, and the ios_base::failure
// type that derives from it.  boost/container_hash names category(),
// default_error_condition() and value(); boost/log names
// basic_ostream<Traits>::failure and relies on the exceptions mask.
#include <sstream>
#include <system_error>

typedef std::basic_ostream<char>::failure stream_failure;
typedef std::basic_ostream<char>::Init stream_init;
typedef std::basic_ostream<char>::event stream_event;
typedef std::basic_ostream<char>::event_callback stream_event_callback;

static int stream_callback_events;

static void stream_callback(stream_event event, std::ios_base &stream, int index)
{
  (void)stream;
  if (index == 7 && event == std::ios_base::imbue_event)
    stream_callback_events |= 1;
}

int main()
{
  stream_init init;
  std::error_code code = std::make_error_code(std::errc::invalid_argument);
  if (!code || code.value() != 22)
    return 1;
  if (&code.category() != &std::generic_category())
    return 2;
  if (code.message().empty())
    return 3;

  std::error_condition condition = code.default_error_condition();
  if (&condition.category() != &std::generic_category() || condition.value() != 22)
    return 4;
  if (code != condition)
    return 5;

  if (&std::system_category() == &std::generic_category())
    return 6;
  if (std::system_category().name() == 0 || std::system_category().name()[0] == 0)
    return 7;

  try
  {
    throw std::system_error(code, "invalid");
  }
  catch (const std::system_error &error)
  {
    if (error.code() != code)
      return 8;
    if (error.what() == 0 || error.what()[0] == 0)
      return 9;
  }

  std::ostringstream stream;
  stream.register_callback(stream_callback, 7);
  stream.imbue(std::locale());
  if (!(stream_callback_events & 1))
    return 13;

  stream.setf(std::ios_base::unitbuf);
  int word_index = std::ostream::xalloc();
  stream.iword(word_index) = 9;
  stream.pword(word_index) = &stream;
  if (stream.iword(word_index) != 9)
    return 15;
  if (stream.pword(word_index) != &stream)
    return 16;
  if (!std::ostream::sync_with_stdio(false))
    return 17;

  stream.exceptions(std::ios_base::badbit);
  if (stream.exceptions() != std::ios_base::badbit)
    return 10;

  std::basic_ostream<char>::sentry sentry(stream);
  if (!sentry)
    return 14;

  try
  {
    stream.setstate(std::ios_base::badbit);
    return 11;
  }
  catch (const stream_failure &error)
  {
    if (error.code() != std::make_error_code(std::io_errc::stream))
      return 12;
  }

  return 0;
}
