struct UnusedText {
  UnusedText(const wchar_t *);
  UnusedText(wchar_t *text) : UnusedText(static_cast<const wchar_t *>(text)) {}
};

int never_defined();
int unused_forwarder();
inline int unused_forwarder() { return never_defined(); }

int called_forwarder();
inline int called_forwarder() { return 17; }

struct UsedText {
  int value;
  explicit UsedText(int initial) : value(initial) {}
  UsedText() : UsedText(called_forwarder()) {}
};

int main()
{
  UsedText value;
  return value.value != 17 || called_forwarder() != 17;
}
