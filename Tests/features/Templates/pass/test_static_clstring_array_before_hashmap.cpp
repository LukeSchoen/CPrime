// EXPECT_COMPILE_ONLY: 1
// EXPECT_MANIFEST_SOURCE: CommonLib/CommonLib/src/Strings/clString.cpp
#include "clString.h"
const clString RO_PrimType_Names[] = { "bool", "byte", "ubyte", "short", "ushort", "double", "uint", "int", "float", "mat2", "mat3", "mat4", "sampler" };
#include "clHashMap.h"

int main()
{
  clHashMap<clString, i64> map;
  map.Add("one", 1);
  return (int)map.at("one") - 1;
}
