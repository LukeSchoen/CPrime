#include <stddef.h>
#include <stdlib.h>
#include <wchar.h>

namespace std
{
  class wstring
  {
  public:
    static const size_t npos = (size_t)-1;

    wstring() : m_data(0), m_len(0) {}
    wstring(const wstring &o) : m_data(0), m_len(0) { assign(o.m_data, o.m_len); }
    explicit wstring(const wchar_t *text) : m_data(0), m_len(0)
    {
      if (text)
        assign(text, wcslen(text));
    }
    ~wstring() { free(m_data); }

    wstring &operator=(const wstring &o)
    {
      if (this != &o)
        assign(o.m_data, o.m_len);
      return *this;
    }

    wstring substr(size_t pos, size_t count = npos) const
    {
      wstring ret;
      size_t len;
      if (pos >= m_len)
        return ret;
      len = m_len - pos;
      if (count < len)
        len = count;
      ret.assign(c_str() + pos, len);
      return ret;
    }

    const wchar_t *c_str() const { return m_data ? m_data : L""; }

  private:
    void assign(const wchar_t *text, size_t len)
    {
      if (m_data)
        free(m_data);
      m_data = (wchar_t *)malloc((len + 1) * sizeof(wchar_t));
      if (m_data)
      {
        for (size_t i = 0; i < len; ++i)
          m_data[i] = text[i];
        m_data[len] = 0;
      }
      m_len = len;
    }

    wchar_t *m_data;
    size_t m_len;
  };
}

int ShellExecuteW(int, const wchar_t *, const wchar_t *, int, int, int)
{
  return 0;
}

class PathLike
{
public:
  std::wstring Directory() const { return std::wstring(L"folder").substr(0); }
};

class WideBridge
{
public:
  static std::wstring ToWideString(std::wstring path) { return path; }
};

void open_path(const PathLike &path)
{
  auto nwide = WideBridge::ToWideString(path.Directory());
  ShellExecuteW(0, L"open", nwide.c_str(), 0, 0, 10);
}

int main()
{
  return 0;
}
