template<class T, class... Args> int choose(T&&, int, Args&&...) { return 1; }
template<class T> int choose(T&&, long) { return 2; }
int main() {
 if (choose(3, 0) != 1 || choose(3, 0L) != 2) return 1;
 if (choose(3, 0, 9) != 1) return 2;
 return 0;
}
