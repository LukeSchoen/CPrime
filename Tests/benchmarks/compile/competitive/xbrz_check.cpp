/* Unmodified xBRZ input copied from CL's raster scaler. Check identical output
   across compilers as well as writing within the destination boundaries. */
#include "xbrz/xBRZ.cpp"
#include <stdio.h>
int main(int argc, char **) {
    uint32_t source[16*16], storage[32*32+2];
    for (unsigned i=0; i<16*16; ++i)
        source[i] = ((i*173u) ^ ((i/16u)*7919u)) & 0xffffffu;
    storage[0]=storage[32*32+1]=0xabcdef12u;
    xbrz::scale(2,source,storage+1,16,16,xbrz::ColorFormat::RGB);
    if (storage[0]!=0xabcdef12u || storage[32*32+1]!=0xabcdef12u) return 1;
    uint32_t hash=2166136261u;
    for (unsigned i=1; i<=32*32; ++i) hash=(hash^storage[i])*16777619u;
    printf("%08x\n",(unsigned)hash);
    if (argc>1) for (unsigned i=1; i<=32*32; ++i) printf("%08x\n",(unsigned)storage[i]);
    return 0;
}
