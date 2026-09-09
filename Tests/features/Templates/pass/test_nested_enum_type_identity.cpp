enum First { first_value };
enum Second { second_value };
template<class T> int* slot() { static int value; return &value; }
int main() {
    return slot<First&>() == slot<int&>()
        || slot<First&>() == slot<Second&>()
        || slot<First*>() == slot<int*>()
        || slot<First*>() == slot<Second*>()
        || slot<First&>() != slot<First&>();
}
