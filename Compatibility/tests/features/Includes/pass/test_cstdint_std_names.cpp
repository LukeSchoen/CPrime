// <cstdint> and <cinttypes> must put their names into namespace std as well
// as the global namespace.

#include <cinttypes>
#include <cstdint>

static_assert(sizeof(std::uint8_t) == 1, "uint8_t size");
static_assert(sizeof(std::uint16_t) == 2, "uint16_t size");
static_assert(sizeof(std::uint32_t) == 4, "uint32_t size");
static_assert(sizeof(std::uint64_t) == 8, "uint64_t size");
static_assert(sizeof(std::intptr_t) == sizeof(void *) || sizeof(std::intptr_t) == 8,
              "intptr_t width");

std::uint16_t width = 4;
std::int64_t offset = -3;
std::uintmax_t limit = 0;

int main()
{
  std::imaxdiv_t parts = std::imaxdiv(7, 2);
  std::uint64_t mask = UINT64_C(0xff);
  return width == 4 && offset == -3 && limit == 0 && parts.quot == 3
         && parts.rem == 1 && mask == 0xff ? 0 : 1;
}
