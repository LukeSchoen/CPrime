struct Empty {};
extern Empty undefined_empty;
int calls;
void consume(Empty) { ++calls; }
Empty* source() { ++calls; return 0; }
struct Custom {
    Custom() {}
    Custom(const Custom&) { ++calls; }
};
int main() {
    Empty value = *source();
    value = *source();
    consume(undefined_empty);
    Custom a;
    Custom b = a;
    return calls != 4;
}
