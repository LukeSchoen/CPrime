int events[16], count;
void note(int value) { if (count < 16) events[count++] = value; }
struct Trace {
    int value;
    Trace(int v) : value(v) { note(v); }
    Trace(const Trace& other) : value(other.value) { note(200 + value); }
    ~Trace() { note(100 + value); }
    Trace operator+(const Trace& other) const { return Trace(value + other.value); }
};
int main() {
    ({ Trace(10) + Trace(11); });
    if (count != 6 || events[2] != 21 || events[5] != 121) return 1;
    if (events[0] + events[1] != 21 || events[3] != events[1] + 100
        || events[4] != events[0] + 100) return 2;
    count = 0;
    ({ Trace local(14); local; });
    if (count != 4 || events[0] != 14 || events[1] != 214
        || events[2] != 114 || events[3] != 114) return 3;
    count = 0;
    ({ Trace local(13); local; ; });
    return count != 2 || events[0] != 13 || events[1] != 113;
}
