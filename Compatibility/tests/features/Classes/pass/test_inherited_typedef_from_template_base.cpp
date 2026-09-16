// A member typedef of a class-template instance is inherited by the classes
// derived from it.  The instance registers the typedef under the alias its
// replayed body declares (traits__unsigned_short__value_type), not under the
// primary template's joined spelling, so an unqualified use inside a class
// derived from the instance has to consult the recorded instance alias.
// Reduced from boost/date_time/gregorian/greg_weekday.hpp.
template<class T> struct traits { typedef T value_type; };

struct weekday_rep : traits<unsigned short>
{
  weekday_rep(unsigned short n) : value_(n) {}
  value_type value_;
};

struct weekday : weekday_rep
{
  weekday(value_type day) : weekday_rep(day) {}
};

int main()
{
  weekday day(3);
  return day.value_ == 3 ? 0 : 1;
}
