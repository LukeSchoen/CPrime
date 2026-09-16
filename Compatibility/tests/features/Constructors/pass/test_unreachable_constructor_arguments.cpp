static int calls;
struct Size { int value; };
struct Image { Size size() const { ++calls; return Size{7}; } };
struct Window { Window(Size) { ++calls; } };
int main() {
    Image image;
    if (false) { Window window(image.size()); }
    if (0) { Window window(Size{3}); }
    return calls;
}
