// EXPECT_COMPILE_ONLY: 1
// EXPECT_MANIFEST_SOURCE: CommonLib/CommonLib/src/Raster/clImage.cpp
#include "windows.h"
#include "clImage.h"

bool clImage::LoadImageMemory(const void *, const i64 &, const bool &)
{
    return false;
}

bool LoadMemoryProbe(clImage &image, const void *data, const i64 &size)
{
    return image.LoadImageMemory(data, size);
}
