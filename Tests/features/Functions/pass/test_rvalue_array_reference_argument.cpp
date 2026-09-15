int accept(int (&&array)[2]) { return array[0] + array[1]; }
int main() { int array[2] = {3, 4}; return accept(static_cast<int (&&)[2]>(array)) != 7; }
