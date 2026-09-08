// EXPECT_COMPILE_ARGS: -Werror
template<class T> T clamp_value(T value, T low, T high) {
  return value < low ? low : value > high ? high : value;
}
template<class T> struct Buffer {
  T pixels[3];
  T *begin() { return pixels; }
  T *end() { return pixels + 3; }
};
struct Image : Buffer<int> {};
// Copying this derived class demands base-class members while scalar function
// template declarations are waiting to be emitted.
Image copied_image(Image input) {
  Image result = input;
  for (auto& pixel : result) pixel = 1;
  return result;
}
float bounded_value(float value) { return clamp_value(value, 0.f, 255.f); }
int main() {
  Image input;
  input.pixels[0] = 7; input.pixels[1] = 8; input.pixels[2] = 9;
  Image result = copied_image(input);
  return result.pixels[0] != 1 || result.pixels[2] != 1 || input.pixels[0] != 7
      || bounded_value(-1.f) != 0.f || bounded_value(42.f) != 42.f
      || bounded_value(300.f) != 255.f;
}
