#include <shellscalingapi.h>

int main()
{
  PROCESS_DPI_AWARENESS awareness = PROCESS_SYSTEM_DPI_AWARE;
  if (awareness != 1) return 1;
    DEVICE_SCALE_FACTOR factor = SCALE_150_PERCENT;
    return factor == 150 ? 0 : 1;
}
