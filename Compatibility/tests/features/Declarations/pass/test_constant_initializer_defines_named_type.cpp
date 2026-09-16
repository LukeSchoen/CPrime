// EXPECT_EXIT: 0
/* A constant initializer is probed with the real parser before the emitting
   replay runs.  A tag or enumerator the probe met for the first time stayed
   registered in the enclosing scope, so the replay of the same tokens
   reported `struct/union/enum 'S' already defined` or `redeclaration of 'a'`.
   The replay now reuses what the probe of the same declaration defined. */

struct S
{
    int a;
    double b;
};

const int struct_size = sizeof (struct ProbeS { int a; double b; });
ProbeS probe_struct;

const int enum_size = sizeof (enum ProbeE { first = 3, second = 5 });
ProbeE probe_enum;

const int anonymous_enum_size = sizeof (enum { anon_first, anon_second });

/* The initializer of an out-of-class static data member is parsed in the
   member's class scope before the file-scope definition is registered. */
struct Owner
{
    static const int named;
    static const int anonymous;
};

const int Owner::named = sizeof (struct MemberS { char bytes[7]; });
const int Owner::anonymous = sizeof (enum { member_first, member_second });
MemberS member_s;

/* An aggregate initializer is probed through the same path. */
struct Holder
{
    int v;
};

const Holder holder = { sizeof (struct Nested { int q; }) };
Nested nested;

int main()
{
    if (struct_size != (int)sizeof(S))
        return 1;
    if (sizeof(ProbeS) != (int)sizeof(S))
        return 2;
    if (enum_size != (int)sizeof(ProbeE))
        return 3;
    if (first != 3 || second != 5)
        return 4;
    if (probe_enum != (ProbeE)0)
        return 5;
    if (anonymous_enum_size != 4)
        return 6;
    if (anon_first != 0 || anon_second != 1)
        return 7;
    if (Owner::named != 7 || Owner::anonymous != 4)
        return 8;
    if (sizeof(MemberS) != 7 || member_s.bytes[0] != 0)
        return 9;
    if (holder.v != (int)sizeof(Nested))
        return 10;
    if (sizeof(Nested) != 4 || nested.q != 0)
        return 11;
    return 0;
}
