/* Static complex initializers must retain both constant parts instead of
   lowering their expression through a runtime temporary. */

static _Complex double scalar = 3.0;
static _Complex double sum = 1.0 + 2.0i;
static _Complex double product = (1.0 + 2.0i) * (3.0 + 4.0i);
static _Complex float converted = 4.0f + 3.0fi;

int main(void)
{
  if (__real__ scalar != 3.0 || __imag__ scalar != 0.0) return 1;
  if (__real__ sum != 1.0 || __imag__ sum != 2.0) return 2;
  if (__real__ product != -5.0 || __imag__ product != 10.0) return 3;
  if (__real__ converted != 4.0f || __imag__ converted != 3.0f) return 4;
  return 0;
}
