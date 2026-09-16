/* A class argument reaches a class parameter through a conversion operator
   as well as through a converting constructor. The conversion was ranked as
   viable and then discarded by the viability predicate, so the assignment
   reported "no matching user-declared copy assignment operator" instead of
   selecting the operator= that takes the converted temporary. */

struct temp_string {
    int tag;
    temp_string (const unsigned char c) { tag = c; }
};

static temp_string shared (0);

struct String {
    int tag;
    String () { tag = -1; }
    String &operator = (temp_string value) { tag = value.tag; return *this; }
    String &operator = (const String &other) { tag = other.tag + 100; return *this; }
};

struct Source {
    int value;
    operator temp_string & () const;
};

temp_string &Source::operator temp_string & () const
{
    shared.tag = value;
    return shared;
}

/* The conversion must also reach an ordinary member function parameter. */
struct Sink {
    int seen;
    Sink () { seen = 0; }
    void put (temp_string value) { seen = value.tag; }
};

int main ()
{
    String destination;
    Source source;
    source.value = 7;
    destination = source;
    if (destination.tag != 7) return 1;

    Sink sink;
    sink.put (source);
    if (sink.seen != 7) return 2;

    String copy;
    copy.tag = 3;
    destination = copy;
    if (destination.tag != 103) return 3;

    return 0;
}
