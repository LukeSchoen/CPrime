namespace outer { namespace inner { enum class Color { Red = 3, Blue = 7 }; } }
int main() {
  bool red = true;
  outer::inner::Color a = red ? outer::inner::Color::Red : outer::inner::Color::Blue;
  outer::inner::Color b = !red ? outer::inner::Color::Red : outer::inner::Color::Blue;
  return (int)a != 3 || (int)b != 7;
}
