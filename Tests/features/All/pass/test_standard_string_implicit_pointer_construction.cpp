#include <string>
static int narrow(const std::string& text) { return text.size()!=4 || text[0]!='w' || text[3]!='d'; }
static int wide(const std::wstring& text) { return text.size()!=4 || text[0]!=L'w' || text[3]!=L'd'; }
int main() {
    const char* chars="word";
    const wchar_t* wide_chars=L"word";
    std::string text=chars;
    std::wstring wide_text=wide_chars;
    return narrow(chars) || wide(wide_chars) || narrow(text) || wide(wide_text);
}
