int main() {
  int result = 0;
  try { throw 17; }
  catch (double) { return 1; }
  catch (int value) { result = value; }
  if (result != 17) return 2;
  try { throw 2.75; }
  catch (int) { return 3; }
  catch (const double &value) { if (value != 2.75) return 4; }
  try { throw 91L; }
  catch (int) { return 5; }
  catch (...) { result += 3; }
  return result == 20 ? 0 : 6;
}
