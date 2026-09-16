#include <sal.h>

struct GapBuffer
{
	_Field_size_opt_(GapCount) unsigned char *Data;
};

_Return_type_success_(return == 0)
enum GapStatus
{
	GapOk = 0,
	GapFailed = 1
};

_Out_writes_bytes_(size) _Out_writes_to_(size, *written)
static int write_gap_bytes(unsigned char *data, unsigned size, unsigned *written)
{
	_Out_range_(>=, 0) int result = size != 0;
	if (written)
		*written = result;
	(void)data;
	return result;
}

int main()
{
	GapBuffer buffer = {0};
	unsigned written = 0;
	return buffer.Data == 0
	       && write_gap_bytes((unsigned char *)&written, sizeof written, &written) != 0
	       && written != 0 ? 0 : 1;
}
