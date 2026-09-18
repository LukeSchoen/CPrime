#include <system_error>
#include <string>

namespace
{
  const char *generic_message(int value)
  {
    switch (value)
    {
    case 0: return "Success";
    case 1: return "Operation not permitted";
    case 2: return "No such file or directory";
    case 3: return "No such process";
    case 4: return "Interrupted system call";
    case 5: return "Input/output error";
    case 6: return "No such device or address";
    case 7: return "Argument list too long";
    case 8: return "Executable format error";
    case 9: return "Bad file descriptor";
    case 10: return "No child process";
    case 11: return "Resource unavailable, try again";
    case 12: return "Not enough memory";
    case 13: return "Permission denied";
    case 14: return "Bad address";
    case 16: return "Device or resource busy";
    case 17: return "File exists";
    case 18: return "Cross-device link";
    case 19: return "No such device";
    case 20: return "Not a directory";
    case 21: return "Is a directory";
    case 22: return "Invalid argument";
    case 23: return "Too many files open in system";
    case 24: return "Too many files open";
    case 25: return "Inappropriate ioctl for device";
    case 26: return "Text file busy";
    case 27: return "File too large";
    case 28: return "No space left on device";
    case 29: return "Invalid seek";
    case 30: return "Read-only file system";
    case 31: return "Too many links";
    case 32: return "Broken pipe";
    case 33: return "Numerical argument out of domain";
    case 34: return "Numerical result out of range";
    case 35: return "Resource deadlock would occur";
    case 36: return "Filename too long";
    case 37: return "No locks available";
    case 38: return "Function not implemented";
    case 39: return "Directory not empty";
    case 40: return "Too many levels of symbolic links";
    case 42: return "No message of desired type";
    case 43: return "Identifier removed";
    case 60: return "Device not a stream";
    case 61: return "No data available";
    case 62: return "Timer expired";
    case 63: return "Out of streams resources";
    case 67: return "Link has been severed";
    case 71: return "Protocol error";
    case 74: return "Bad message";
    case 75: return "Value too large for defined data type";
    case 84: return "Invalid or incomplete multibyte or wide character";
    case 88: return "Socket operation on non-socket";
    case 89: return "Destination address required";
    case 90: return "Message too long";
    case 91: return "Protocol wrong type for socket";
    case 92: return "Protocol not available";
    case 93: return "Protocol not supported";
    case 95: return "Operation not supported";
    case 97: return "Address family not supported by protocol";
    case 98: return "Address already in use";
    case 99: return "Cannot assign requested address";
    case 100: return "Network is down";
    case 101: return "Network is unreachable";
    case 102: return "Network dropped connection on reset";
    case 103: return "Software caused connection abort";
    case 104: return "Connection reset by peer";
    case 105: return "No buffer space available";
    case 106: return "Transport endpoint is already connected";
    case 107: return "Transport endpoint is not connected";
    case 110: return "Connection timed out";
    case 111: return "Connection refused";
    case 113: return "No route to host";
    case 114: return "Operation already in progress";
    case 115: return "Operation now in progress";
    case 125: return "Operation canceled";
    case 130: return "Owner died";
    case 131: return "State not recoverable";
    default: return "Generic error";
    }
  }

  class generic_error_category_impl : public std::error_category
  {
  public:
    const char *name() const noexcept override { return "generic"; }
    std::string message(int value) const override { return std::string(generic_message(value)); }
  };

  class system_error_category_impl : public std::error_category
  {
  public:
    const char *name() const noexcept override { return "system"; }
    std::string message(int value) const override
    {
      return std::string(value == 0 ? "Success" : "System error");
    }
  };

  class iostream_error_category_impl : public std::error_category
  {
  public:
    const char *name() const noexcept override { return "iostream"; }
    std::string message(int value) const override
    {
      return std::string(value == 0 ? "Success" : "iostream error");
    }
  };

  generic_error_category_impl generic_category_object;
  system_error_category_impl system_category_object;
  iostream_error_category_impl iostream_category_object;
}

namespace std
{
  int __cpc_ios_base_xalloc() noexcept
  {
    static int next_index;
    return next_index++;
  }

  error_condition error_code::default_error_condition() const noexcept
  {
    return m_category->default_error_condition(m_value);
  }

  string error_code::message() const
  {
    return m_category->message(m_value);
  }

  string error_condition::message() const
  {
    return m_category->message(m_value);
  }

  error_condition error_category::default_error_condition(int value) const noexcept
  {
    return error_condition(value, *this);
  }

  bool error_category::equivalent(int value, const error_condition &condition) const noexcept
  {
    return default_error_condition(value) == condition;
  }

  bool error_category::equivalent(const error_code &code, int value) const noexcept
  {
    return *this == code.category() && code.value() == value;
  }

  const error_category &generic_category() noexcept
  {
    return generic_category_object;
  }

  const error_category &system_category() noexcept
  {
    return system_category_object;
  }

  const error_category &iostream_category() noexcept
  {
    return iostream_category_object;
  }
}
