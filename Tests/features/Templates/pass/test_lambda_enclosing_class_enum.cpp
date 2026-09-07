struct Editor {
 enum class Color { Default, Max };
 struct Language { static int run(); };
};
int Editor::Language::run() {
 auto tokenize = [](Color& color) -> bool { color = Color::Max; return color != Color::Default; };
 Color color = Color::Default;
 return tokenize(color) && color == Color::Max ? 0 : 1;
}
int main() { return Editor::Language::run(); }
