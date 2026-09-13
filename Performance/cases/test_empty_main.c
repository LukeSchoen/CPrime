// PERF_NAME: c.empty.main
/* Floor of the suite: process start, driver setup, one translation unit with
   no work. Nothing here should ever dominate a slower case, so a rise in this
   row means fixed per-invocation cost grew. */

int main(void)
{
  return 0;
}
