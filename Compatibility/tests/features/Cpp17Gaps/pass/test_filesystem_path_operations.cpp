// EXPECT_COMPILE_ARGS: -std=c++17
// CL gap probe: filesystem_path. std::filesystem::path exposes only c_str and
// empty; extension, filename, parent_path and operator/ are missing.

#include <filesystem>

int main()
{
  std::filesystem::path file("dir/name.txt");
  if (file.extension() != ".txt") return 1;
  if (file.filename() != "name.txt") return 2;
  return file.parent_path() == std::filesystem::path("dir") ? 0 : 3;
}
