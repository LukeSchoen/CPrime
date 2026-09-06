// EXPECT_MANIFEST_SOURCE: CommonLib/CommonLib/src/Math/Geometry/clKNN3.cpp
#include "clVector4.h"
struct State { clVector4<bool> value; State():value({true,false,true,false}){} };
clVector2<clVector3<long long>> date(){return {{2026LL,9LL,6LL},{16LL,30LL,12LL}};}
int main(){State s;auto d=date();return s.value.x&&!s.value.y&&s.value.z&&!s.value.w && d.x.x==2026 && d.y.z==12?0:1;}
