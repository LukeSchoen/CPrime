#include <iosfwd>
#include <istream>
#include <ostream>
#include <stdio.h>
int main() {
  FILE* f = tmpfile();
  if (!f) return 1;
  std::ostream out(f);
  out.write("abc", 3).flush();
  std::istream in(f);
  in.seekg(0);
  char text[5] = {};
  in.read(text, 4);
  if (in.gcount() != 3 || !in.eof() || !in.fail() || in.bad()) return 2;
  in.clear();
  in.seekg(1, std::ios::beg);
  if (in.tellg() != 1) return 3;
  in.read(text, 2);
  if (!in || text[0] != 'b' || text[1] != 'c') return 4;
  fclose(f);
  return 0;
}
