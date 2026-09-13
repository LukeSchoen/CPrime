// PERF_NAME: c.string.literals
/* Read-only data volume: many string literals, char arrays and a static table,
   which exercises literal interning, section layout and relocation. */

static const char *const names[] = {
  "alpha", "beta", "gamma", "delta", "epsilon", "zeta", "eta", "theta",
  "iota", "kappa", "lambda", "mu", "nu", "xi", "omicron", "pi",
  "rho", "sigma", "tau", "upsilon", "phi", "chi", "psi", "omega",
  "red", "green", "blue", "cyan", "magenta", "yellow", "black", "white",
  "one", "two", "three", "four", "five", "six", "seven", "eight",
  "nine", "ten", "eleven", "twelve", "thirteen", "fourteen", "fifteen",
  "sixteen", "seventeen", "eighteen", "nineteen", "twenty"
};

static const char banner[] =
  "cprime performance case: string literals, concatenated "
  "across several source lines so the lexer has to join them "
  "into one object and the writer has to place it in read-only data";

static const unsigned char bytes[64] = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
  16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
  32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
  48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63
};

int main(void)
{
  int i;
  int total = 0;
  for (i = 0; i < 52; i++) {
    const char *name = names[i];
    while (*name) {
      total += *name;
      name++;
    }
  }
  for (i = 0; i < (int)sizeof(banner) - 1; i++)
    total += banner[i];
  for (i = 0; i < 64; i++)
    total += bytes[i];
  return total == 0 ? 1 : 0;
}
