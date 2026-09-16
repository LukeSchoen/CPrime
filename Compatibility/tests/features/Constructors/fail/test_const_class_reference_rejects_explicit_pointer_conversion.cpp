// EXPECT_COMPILE_FAIL: 1
struct Word { explicit Word(const wchar_t*) {} };
void consume(const Word&) {}
int main() { const wchar_t* text=L"word"; consume(text); }
