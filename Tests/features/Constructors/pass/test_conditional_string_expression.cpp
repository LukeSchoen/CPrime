#include <stdio.h>
#include <string.h>
struct Text {
  char data[64];
  Text(const char *s) { strcpy(data, s); }
  Text(int value) { sprintf(data, "%d", value); }
  bool operator!=(const char *s) const { return strcmp(data, s) != 0; }
};
Text operator+(const char *prefix, const Text &value) {
  Text result(prefix);
  strcat(result.data, value.data);
  return result;
}
Text choose(bool condition,int value){return condition?("number " + Text(value)):"";}
int main(){return choose(true,7)!="number 7" || choose(false,7)!="";}
