// EXPECT_COMPILE_ONLY: 1
// EXPECT_MANIFEST_SOURCE: CommonLib/CommonLib/src/Raster/clImage.cpp
// Reproduce the header order in a combined histogram/image translation unit.
#include "clHistogram.h"
#include "windows.h"

bool clImage::LoadImageMemory(const void *, const i64 &, const bool &)
{
    return false;
}

bool LoadMemoryProbe(clImage &image, const void *data, const i64 &size)
{
    return image.LoadImageMemory(data, size);
}
