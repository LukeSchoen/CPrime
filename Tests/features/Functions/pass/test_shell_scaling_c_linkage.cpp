// EXPECT_COMPILE_ARGS: -Werror -lshcore
#include <shellscalingapi.h>
HRESULT (WINAPI* volatile monitor_function)(HMONITOR, DEVICE_SCALE_FACTOR*) =
    &GetScaleFactorForMonitor;
HRESULT (WINAPI* volatile awareness_function)(PROCESS_DPI_AWARENESS) =
    &SetProcessDpiAwareness;
int main() { return !monitor_function || !awareness_function; }
