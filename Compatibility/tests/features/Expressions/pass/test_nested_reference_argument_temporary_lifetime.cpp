const int& minimum(const int& left, const int& right) {
    return left < right ? left : right;
}
int check(int left, int right) { return left != 1 || right != 100; }
const double& identity(const double& value) { return value; }
int check_double(double left, double right) { return left != 1.5 || right != 8.5; }
int main() {
    if (check(minimum(2, 1), minimum(100, 200))) return 1;
    return check_double(identity(1.5), identity(8.5));
}
