// EXPECT_EXIT: 0
// A constructor member-initializer that value-initializes a member whose class
// type is an anonymous struct introduced through a typedef used to be emitted
// as an empty assignment (`this->_M_st = ;`), so `_M_st()` was rejected with
// `expression expected before ';'`.  The member has no name token to build the
// placement-construction path from, so the initializer must fall back to the
// brace form and still zero-initialize the member.
typedef struct {
    int count;
    int extra;
} mbstate_t;

struct fpos {
    mbstate_t _M_st;
    int tag;
    fpos(int pos) : _M_st(), tag(pos) { }
};

int main() {
    unsigned char storage[sizeof(fpos)];
    unsigned i;
    for (i = 0; i < sizeof(storage); ++i)
        storage[i] = 0xAB;
    fpos* p = new (storage) fpos(2);
    if (p->_M_st.count != 0)
        return 1;
    if (p->_M_st.extra != 0)
        return 2;
    if (p->tag != 2)
        return 3;
    return 0;
}
