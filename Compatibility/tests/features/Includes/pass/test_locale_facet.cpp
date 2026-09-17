#include <locale>
#include <sstream>
#include <string>

/* The date facets of boost/date_time derive from std::locale::facet, publish a
   static std::locale::id, and are registered with
   std::locale(base, new facet) before the stream is asked for them with
   getloc/has_facet/use_facet. */
class date_like_facet : public std::locale::facet
{
public:
  static std::locale::id id;

  date_like_facet() : std::locale::facet(0), m_marker(0) {}

  int marker() const { return m_marker; }
  void marker(int value) { m_marker = value; }

private:
  int m_marker;
};

std::locale::id date_like_facet::id;

class other_facet : public std::locale::facet
{
public:
  static std::locale::id id;
};

std::locale::id other_facet::id;

int main()
{
  std::ostringstream out;
  if (std::has_facet<date_like_facet>(out.getloc()))
    return 1;

  date_like_facet* added = new date_like_facet();
  added->marker(7);
  std::locale extended(out.getloc(), added);
  out.imbue(extended);

  if (!std::has_facet<date_like_facet>(out.getloc()))
    return 2;
  if (std::use_facet<date_like_facet>(out.getloc()).marker() != 7)
    return 3;
  if (std::has_facet<other_facet>(out.getloc()))
    return 4;

  out << "abc";
  if (out.str() != "abc")
    return 5;

  std::locale copied=out.getloc();
  if (!std::has_facet<date_like_facet>(copied))
    return 6;

  std::istringstream in(std::string("x"));
  in.imbue(copied);
  if (!std::has_facet<date_like_facet>(in.getloc()))
    return 7;

  date_like_facet* replacement = new date_like_facet();
  replacement->marker(9);
  std::locale replaced(out.getloc(), replacement);
  if (std::use_facet<date_like_facet>(replaced).marker() != 9)
    return 8;

  return 0;
}
