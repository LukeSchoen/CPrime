// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
int destroyed, bad_dummy, receiver_calls;
struct Snapshot {
  int value;
  Snapshot(int input) : value(input) {}
  ~Snapshot() { ++destroyed; }
};
struct Counter {
  int value;
  Counter(int input) : value(input) {}
  Counter &operator++() { ++value; return *this; }
  Counter &operator--() { --value; return *this; }
  Snapshot operator++(int dummy) {
    bad_dummy += dummy;
    return Snapshot(value++);
  }
  Snapshot operator--(int dummy) {
    bad_dummy += dummy;
    return Snapshot(value--);
  }
};
Counter &receiver(Counter &counter) { ++receiver_calls; return counter; }
struct Plain { int value; };
int operator++(Plain &value, int dummy) { bad_dummy += dummy; return value.value++; }
int operator--(Plain &value, int dummy) { bad_dummy += dummy; return value.value--; }

int main() {
  Counter counter(10);
  if (receiver(counter)++.value != 10 || counter.value != 11) return 1;
  if (destroyed != 1 || receiver_calls != 1) return 2;
  if (counter--.value != 11 || counter.value != 10) return 3;
  if (destroyed != 2) return 3;
  if ((++counter).value != 11 || (--counter).value != 10) return 4;
  Plain plain = { 20 };
  if (plain++ != 20 || plain-- != 21 || plain.value != 20) return 5;
  return bad_dummy ? 6 : 0;
}
