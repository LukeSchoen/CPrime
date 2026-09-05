// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
typedef enum { key_zero, key_one } Key;
typedef enum { other_zero, other_one } Other;
struct Controls {
    static bool down(const Key& key);
    Key echo(Key key) const;
    Other echo(Other key) const;
};
bool Controls::down(const Key& key) { return key == key_one; }
Key Controls::echo(Key key) const { return key; }
Other Controls::echo(Other key) const { return key; }
int main() {
    Controls controls;
    Key key = key_zero;
    if (!Controls::down(key_one) || Controls::down(key)) return 1;
    if (controls.echo(key_one) != key_one) return 2;
    return controls.echo(other_one) != other_one;
}
