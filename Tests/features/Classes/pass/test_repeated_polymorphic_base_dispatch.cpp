// EXPECT_EXIT: 0
int destroyed;
struct Root { virtual ~Root() {} virtual int read() { return 1; } };
struct Left : Root {
  int read() { return 3; }
  virtual int left_only() { return 13; }
};
struct Right : Root {
  int read() { return 5; }
  virtual int right_only() { return 17; }
};
struct Object : Left, Right {
  ~Object() { ++destroyed; }
  int read() { return 7; }
  int right_only() { return 19; }
};
struct Split : Left, Right {};
int main() {
  Object object;
  Root* left = static_cast<Left*>(&object);
  Root* right = static_cast<Right*>(&object);
  if (left == right) return 1;
  if (left->read() != 7) return 2;
  if (right->read() != 7) return 3;
  Left* left_branch = &object;
  Right* right_branch = &object;
  if (left_branch->left_only() != 13 || right_branch->right_only() != 19) return 8;
  if (dynamic_cast<void*>(left) != &object
      || dynamic_cast<void*>(right) != &object) return 5;
  if (dynamic_cast<Right*>(left) != static_cast<Right*>(&object)) return 6;
  Split split;
  left = static_cast<Left*>(&split);
  right = static_cast<Right*>(&split);
  if (left->read() != 3 || right->read() != 5) return 4;
  delete static_cast<Left*>(new Object);
  delete static_cast<Right*>(new Object);
  if (destroyed != 2) return 7;
  return 0;
}
