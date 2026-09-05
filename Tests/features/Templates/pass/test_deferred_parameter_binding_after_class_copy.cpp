// EXPECT_COMPILE_ONLY: 1
// EXPECT_MANIFEST_SOURCE: CommonLib/CommonLib/src/Raster/clHistogram.cpp
// EXPECT_COMPILE_ARGS: -Werror
#include "clImage.h"

// Copying this derived class demands additional base-class member signatures
// while scalar function-template declarations are waiting to be emitted.
clImage copied_image(clImage input) {
  clImage result = input;
  for (auto& pixel : result) pixel = 1;
  return result;
}

float bounded_value(float value) {
  return clClamp(value, 0.f, 255.f);
}
