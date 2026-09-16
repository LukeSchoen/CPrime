// EXPECT_EXIT: 0
// Keep macro and enum lookups valid while the identifier table rehashes.
#define ITEM(a,b,c,d,e) enum { symbol_##a##b##c##d##e = a*10000+b*1000+c*100+d*10+e };
#define TEN(a,b,c,d) ITEM(a,b,c,d,0) ITEM(a,b,c,d,1) ITEM(a,b,c,d,2) ITEM(a,b,c,d,3) ITEM(a,b,c,d,4) ITEM(a,b,c,d,5) ITEM(a,b,c,d,6) ITEM(a,b,c,d,7) ITEM(a,b,c,d,8) ITEM(a,b,c,d,9)
#define HUNDRED(a,b,c) TEN(a,b,c,0) TEN(a,b,c,1) TEN(a,b,c,2) TEN(a,b,c,3) TEN(a,b,c,4) TEN(a,b,c,5) TEN(a,b,c,6) TEN(a,b,c,7) TEN(a,b,c,8) TEN(a,b,c,9)
#define THOUSAND(a,b) HUNDRED(a,b,0) HUNDRED(a,b,1) HUNDRED(a,b,2) HUNDRED(a,b,3) HUNDRED(a,b,4) HUNDRED(a,b,5) HUNDRED(a,b,6) HUNDRED(a,b,7) HUNDRED(a,b,8) HUNDRED(a,b,9)
#define TEN_THOUSAND(a) THOUSAND(a,0) THOUSAND(a,1) THOUSAND(a,2) THOUSAND(a,3) THOUSAND(a,4) THOUSAND(a,5) THOUSAND(a,6) THOUSAND(a,7) THOUSAND(a,8) THOUSAND(a,9)
TEN_THOUSAND(0)
TEN_THOUSAND(1)
#if !defined(ITEM) || !defined(TEN_THOUSAND)
#error identifier rehash lost a macro
#endif
int main(void) {
    return symbol_00000 != 0 || symbol_01234 != 1234
        || symbol_09999 != 9999 || symbol_10000 != 10000
        || symbol_19999 != 19999;
}
