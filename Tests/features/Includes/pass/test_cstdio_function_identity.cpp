#include <cstdio>
namespace std {
int exercise_file() {
 FILE* stream = tmpfile();
 if (!stream) return 1;
 if (fputs("ok", stream) < 0 || fflush(stream) || ftell(stream) != 2) return 2;
 rewind(stream);
 char text[3] = {};
 if (fread(text, 1, 2, stream) != 2 || text[0] != 'o' || text[1] != 'k') return 3;
 return fclose(stream);
}
}
int main() {
 int (*standard_close)(FILE*) = std::fclose;
 int (*global_close)(FILE*) = ::fclose;
 if (standard_close != global_close) return 4;
 return std::exercise_file();
}
