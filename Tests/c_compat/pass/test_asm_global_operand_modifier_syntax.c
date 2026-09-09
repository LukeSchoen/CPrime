// EXPECT_COMPILE_ONLY: 1

extern int values[42], scalar;
void helper(void);

asm ("# %cc0: %cc1: %cc2 %cc3"
     :: ":" (helper), ":" (values), "-s" (scalar), "-i" (&values[1]));

int probe(void)
{
  return scalar;
}
