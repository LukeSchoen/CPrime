typedef enum { BeforeDefinition = 1 } DefinitionFirst;
#include "typedef_record_linkage.h"
bool Controls::down(const KeyCode &key) { return key == KeyRight; }
int Controls::axis(AxisCode axis) { return axis == AxisY ? 9 : 4; }
int Controls::read(const RecordAlias &record, keys::Named key, Nested nested)
{
  return record.value + int(key) + int(nested);
}
