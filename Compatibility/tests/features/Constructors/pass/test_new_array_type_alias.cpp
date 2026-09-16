// EXPECT_EXIT: 0
int constructions, destructions;
struct Value {
    int value;
    Value() : value(++constructions) {}
    ~Value() { ++destructions; }
};
typedef Value Row[3];
typedef Row Matrix[2];
int main() {
    Value* row = new Row;
    if (constructions != 3 || row[2].value != 3) return 1;
    delete[] row;
    Row* matrix = new Matrix;
    if (constructions != 9 || matrix[1][2].value != 9) return 2;
    delete[] matrix;
    matrix = new (Matrix);
    if (constructions != 15 || matrix[1][2].value != 15) return 3;
    delete[] matrix;
    matrix = new Row[2];
    if (constructions != 21 || matrix[1][2].value != 21
        || sizeof(*matrix) != 3 * sizeof(Value)) return 4;
    delete matrix; // GNU pointer-to-array delete extension.
    return destructions != 21;
}
