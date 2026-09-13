static int base_destroyed;
static int result;

struct Base {
    ~Base() { ++base_destroyed; }
};

struct Subject : Base {
    ~Subject();
};

Subject::~Subject() try {
    throw 1;
} catch (...) {
    if (!base_destroyed)
        result = 1;
    return;
}

int main() {
    { Subject value; }
    return result;
}
