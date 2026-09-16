struct Values {
    Values() : count(), amount(), pointer(), flag() {}
    int count;
    double amount;
    int *pointer;
    bool flag;
};
int main() {
    Values values;
    return values.count != 0 || values.amount != 0.0
        || values.pointer != 0 || values.flag;
}
