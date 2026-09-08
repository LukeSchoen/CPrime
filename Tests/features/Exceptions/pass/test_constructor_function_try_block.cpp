int live, caught;
struct Tracker {
  Tracker(int) { ++live; }
  ~Tracker() { --live; }
};
int member_id(bool fail) { if (fail) throw 7; return 2; }
struct Object : Tracker {
  Tracker member;
  Object(bool fail) try : Tracker(1), member(member_id(fail)) { throw 9; }
  catch (int) { caught = live == 0 ? caught + 1 : -100; }
};
struct Outside { Outside(); };
Outside::Outside() try { throw 13; } catch (...) { ++caught; }
int recover() try { throw 11; } catch (int value) { return value; }
template<class T> struct Generic {
  Generic() try { throw T(17); } catch (...) { ++caught; }
};
int main() {
  try { Object object(true); return 1; } catch (int value) { if (value != 7) return 2; }
  try { Object object(false); return 3; } catch (int value) { if (value != 9) return 4; }
  try { Outside outside; return 5; } catch (int value) { if (value != 13) return 6; }
  try { Generic<int> generic; return 7; } catch (int value) { if (value != 17) return 8; }
  return caught == 4 && live == 0 && recover() == 11 ? 0 : 9;
}
