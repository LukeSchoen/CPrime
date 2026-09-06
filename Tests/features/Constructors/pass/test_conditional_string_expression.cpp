// EXPECT_MANIFEST_SOURCE: CommonLib/CommonLib/src/Math/Geometry/clKNN3.cpp
// EXPECT_COMPILE_ONLY: 1
#include "clString.h"
clString choose(bool condition,int value){return condition?("number " + clString(value)):"";}
int main(){return choose(true,7)!="number 7" || choose(false,7)!="";}
