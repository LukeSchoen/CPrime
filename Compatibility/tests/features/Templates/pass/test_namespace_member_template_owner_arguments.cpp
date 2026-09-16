// EXPECT_EXIT: 0
namespace {
template<unsigned Stride, unsigned Bias> struct Grid {
    template<unsigned Row, unsigned Col> unsigned index() const {
        return Row * Stride + Col + Bias;
    }
    unsigned origin() const { return Bias; }
    template<unsigned Offset> static unsigned adjusted() { return Bias + Offset; }
};
}
int main() {
    Grid<5, 7> first;
    Grid<5, 11> second;
    if (first.index<2, 3>() != 20) return 1;
    if (second.index<2, 3>() != 24) return 2;
    if (first.index<1, 2>() != 14) return 3;
    if (first.origin() != 7 || second.origin() != 11) return 4;
    if (Grid<5, 7>::adjusted<3>() != 10) return 5;
    if (Grid<5, 11>::adjusted<3>() != 14) return 6;
    return 0;
}
