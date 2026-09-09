struct Entry;
struct Registry {
    static Entry table[];
    int value;
    Registry();
    int action() { return 7; }
    virtual int virtual_action() { return 9; }
};
struct Entry { const char *name; int (Registry::*method)(); int Registry::*data; };
int observed;
Registry registry;
Entry Registry::table[] = {{"action", &Registry::action, &Registry::value},
                           {"virtual", &Registry::virtual_action, nullptr},
                           {"null", nullptr, nullptr}};
Registry::Registry() : value(11) {
    // Both fields require constant initialization before any constructors run.
    if (!table[0].name || table[0].name[0] != 'a') observed = 1;
    else if (!table[0].method || (this->*table[0].method)() != 7) observed = 2;
    else if (!table[1].name || (this->*table[1].method)() != 9) observed = 3;
    else if (!table[2].name || table[2].method) observed = 4;
    else if (this->*table[0].data != 11 || table[1].data || table[2].data) observed = 5;
}
int main() { return observed; }
