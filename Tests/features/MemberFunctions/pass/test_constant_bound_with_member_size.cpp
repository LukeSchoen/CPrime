struct Buffer {
  char memory[192];
  int check() {
    const unsigned long long offset = 0;
    static const char valid[(8 + offset <= sizeof(memory)) ? 1 : -1] = {0};
    return valid[0];
  }
};
int main() { Buffer b; return b.check(); }
