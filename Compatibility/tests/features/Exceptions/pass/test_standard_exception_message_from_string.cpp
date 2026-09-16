// The standard exception classes take both a `const string&` and a
// `const char*` message and keep a copy, so an exception built from a
// temporary string survives the throw and reads back in the handler.
// Reduced from boost::date_time's `bad_weekday`, which derives from
// std::out_of_range and passes a temporary std::string.
#include <stdexcept>
#include <string>

struct bad_weekday : std::out_of_range
{
  bad_weekday() : std::out_of_range(std::string("weekday is out of range 0..6")) {}
};

int main()
{
  try { throw bad_weekday(); }
  catch (const std::out_of_range &error)
  {
    if (std::string(error.what()) != "weekday is out of range 0..6") return 1;
  }
  try { throw std::length_error("too long"); }
  catch (const std::logic_error &error)
  {
    if (std::string(error.what()) != "too long") return 2;
  }
  try { throw std::runtime_error(std::string("late")); }
  catch (const std::exception &error)
  {
    if (std::string(error.what()) != "late") return 3;
  }
  return 0;
}
