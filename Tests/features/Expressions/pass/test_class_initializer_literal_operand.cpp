struct Text {
  int length;
  Text(const char *text) : length(0) { while (text[length]) ++length; }
  Text(int count) : length(count) {}
  Text(const Text &other) : length(other.length) {}
};

Text operator+(const char *prefix, const Text &suffix) {
  Text first(prefix);
  return Text(first.length + suffix.length);
}

int main() {
  Text suffix("beta");
  Text message = "alpha " + suffix;
  if (message.length != 10) return 1;
  Text empty = "";
  return empty.length != 0;
}
