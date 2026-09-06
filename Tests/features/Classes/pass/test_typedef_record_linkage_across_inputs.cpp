// EXPECT_SOURCES: ["typedef_record_linkage_other.cpp"]
typedef enum { BeforeCall = 2 } CallFirst;
typedef enum { BeforeCallAgain = 5 } CallSecond;
#include "typedef_record_linkage.h"
typedef Record CallRecordAlias;
int later_record_value(const CallRecordAlias &record);
int main()
{
  Record record = {17};
  return !Controls::down(KeyRight) || Controls::axis(AxisY) != 9
    || Controls::read(record, keys::Selected, Controls::Inner) != 41
    || later_record_value(record) != 17;
}
