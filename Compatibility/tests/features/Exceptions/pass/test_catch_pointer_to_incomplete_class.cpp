// A pointer chain may name an incomplete class: matching compares the
// pointed-to type identity and qualifiers, never the pointee's size.
struct B;
struct C;

static int caught;

static void raise() {
  try {
    throw (B **)0;
  } catch (C **) {
    caught = 1;
  } catch (B **) {
    caught = 2;
  } catch (...) {
    caught = 3;
  }
}

int main() {
  raise();
  return caught == 2 ? 0 : 1;
}
