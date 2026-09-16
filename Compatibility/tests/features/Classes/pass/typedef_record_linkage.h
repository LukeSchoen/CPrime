typedef enum { KeyLeft = 3, KeyRight = 7 } KeyCode;
typedef enum { AxisX = 0, AxisY = 1 } AxisCode;
typedef struct { int value; } Record;
typedef Record RecordAlias;
namespace keys {
  typedef enum { Selected = 11 } Named;
}
struct Controls {
  typedef enum { Inner = 13 } Nested;
  static bool down(const KeyCode &key);
  static int axis(AxisCode axis);
  static int read(const RecordAlias &record, keys::Named key, Nested nested);
};
