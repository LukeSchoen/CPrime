// EXPECT_EXIT: 0
/* A conditional whose result is an lvalue joins its arms as addresses, so a
   file-scope integral constant that names one arm has to stay an address: the
   value fold used to replace the arm with the constant itself, which left the
   shared load with a pointer no code had produced, and the first read through
   it faulted.  The shape is a threshold ternary inside an allocator,
   `size <= threshold ? page_size : size`. */
#include <cstddef>

static const size_t page_size = 32704;
static const size_t exhaustion_limit = 8000;

static size_t wanted_size(size_t size)
{
  return size <= exhaustion_limit ? page_size : size;
}

static size_t ordinary_size;

/* The condition stays a runtime value, so the conditional keeps both arms and
   has to select one of the two objects. */
static const size_t &selected_size(bool small)
{
  return small ? page_size : ordinary_size;
}

static const int floor_value = 3;
static int ceiling_value = 9;

int main()
{
  if (wanted_size(64) != 32704) return 1;
  if (wanted_size(32768) != 32768) return 2;

  if (&selected_size(true) != &page_size) return 3;
  if (&selected_size(false) != &ordinary_size) return 4;

  /* The conditional is an lvalue, so the selected arm keeps its identity. */
  int low = floor_value;
  (low < ceiling_value ? low : ceiling_value) = 7;
  if (low != 7 || ceiling_value != 9) return 5;

  return 0;
}
