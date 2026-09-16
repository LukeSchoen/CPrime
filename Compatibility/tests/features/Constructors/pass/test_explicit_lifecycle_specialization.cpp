// EXPECT_EXIT: 0
int calls;
template<class T> struct Record {
    Record(int);
    ~Record();
};
template<> Record<int>::Record(int value) { calls += value; }
template<> Record<int>::~Record() { calls += 10; }
template<class T> struct Copy {
    Copy(const T&) { ++calls; }
};
template Copy<double>::Copy(const double&);
int main() {
    { Record<int> record(4); if (calls != 4) return 1; }
    Copy<double> copy(1.0);
    return calls != 15;
}
