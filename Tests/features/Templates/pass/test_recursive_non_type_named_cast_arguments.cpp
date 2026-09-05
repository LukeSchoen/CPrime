// EXPECT_EXIT: 0
enum Rotation { R0, R90, R180, R270 };

template<Rotation R, unsigned Row, unsigned Col, unsigned Size> struct Matrix;
template<unsigned Row, unsigned Col, unsigned Size>
struct Matrix<R0, Row, Col, Size> {
    static const unsigned row = Row;
    static const unsigned col = Col;
};
template<Rotation R, unsigned Row, unsigned Col, unsigned Size>
struct Matrix {
    static const unsigned row = Size - 1 - Matrix<static_cast<Rotation>(R - 1), Row, Col, Size>::col;
    static const unsigned col = Matrix<static_cast<Rotation>(R - 1), Row, Col, Size>::row;
};

int main() {
    if (Matrix<R0, 1, 2, 5>::row != 1 || Matrix<R0, 1, 2, 5>::col != 2) return 1;
    if (Matrix<R90, 1, 2, 5>::row != 2 || Matrix<R90, 1, 2, 5>::col != 1) return 2;
    if (Matrix<R180, 1, 2, 5>::row != 3 || Matrix<R180, 1, 2, 5>::col != 2) return 3;
    if (Matrix<R270, 1, 2, 5>::row != 2 || Matrix<R270, 1, 2, 5>::col != 3) return 4;
    if (Matrix<static_cast<Rotation>(static_cast<unsigned>(R90) + 1), (1 + 1), (2 - 1), 4>::row != 1) return 5;
    return 0;
}
