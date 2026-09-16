// The x86_64 'Q' constraint selects the byte-addressable general registers,
// which is what an in-place byte swap through the %b/%h operand modifiers
// needs (SDL's x86_64 SDL_Swap16 spells it that way).

typedef unsigned short Uint16;

static inline Uint16 swap16(Uint16 x)
{
  __asm__("xchgb %b0,%h0" : "=Q"(x) : "0"(x));
  return x;
}

int main()
{
  return swap16(0x1234) == 0x3412 && swap16(0x00ff) == 0xff00 ? 0 : 1;
}
