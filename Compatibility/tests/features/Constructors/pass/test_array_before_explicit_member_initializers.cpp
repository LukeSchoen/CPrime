// EXPECT_COMPILE_ARGS: -Werror
int constructed;
struct Block {
  int order;
  Block() : order(++constructed) {}
};
struct View {
  Block* block;
  bool* error;
  View(Block* value, bool* failure = 0) : block(value), error(failure) {
    ++constructed;
  }
};
struct Owner {
  Block blocks[2][2];
  View first;
  View last;
  bool failed;
  int count = ++constructed;
  Owner() : first(&blocks[0][0], &failed), last(&blocks[1][1], &failed), failed(false) {}
};
int main() {
  Owner value;
  if (constructed != 7 || value.count != 7) return 1;
  if (value.blocks[0][0].order != 1 || value.blocks[1][1].order != 4) return 2;
  if (value.first.block != &value.blocks[0][0] || value.last.block != &value.blocks[1][1]) return 3;
  return value.first.error != &value.failed || value.last.error != &value.failed || value.failed;
}
