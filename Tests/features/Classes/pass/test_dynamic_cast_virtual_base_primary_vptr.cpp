// EXPECT_EXIT: 0
// dynamic_cast reads the source vptr for runtime type information.  When the
// source class declares virtual functions and also has its own virtual base,
// its virtual base has a separate vptr.  The source vptr must be taken from
// the source class's primary layout root, not from the first matching virtual
// table entry (which may describe the virtual base).
struct Root {
    int root;
    virtual ~Root() {}
};

struct Left : virtual Root {
    int left;
    virtual void left_method() {}
};

struct Right : virtual Root {
    int right;
    virtual void right_method() {}
};

struct Implementation : Left, Right {
    void left_method() {}
    void right_method() {}
};

int main() {
    Implementation* complete = new Implementation;
    Left* source = complete;
    Implementation* recovered = dynamic_cast<Implementation*>(source);
    if (recovered != complete)
        return 1;
    Root* root = dynamic_cast<Root*>(recovered);
    if (!root)
        return 2;
    recovered->left = 7;
    source->left = 8;
    if (recovered->left != 8)
        return 3;
    return 0;
}
