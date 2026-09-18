/* std::thread binds its arguments and passes them through
   __cpc_thread_arguments<>::invoke -> std::invoke, so a function taking a
   pointer has to work exactly like one taking an int.  The stored argument
   used to lose its pointer type in that chain and the invoke call reported
   "no matching function template '__cpc_ns_std_invoke'". */
// EXPECT_EXIT: 0
#include <thread>

static void *seen;

static void remember(void *value) { seen = value; }

int main()
{
  int token = 0;
  std::thread task(remember, (void *)&token);
  task.join();
  if (seen != &token) return 1;

  std::thread null_task(remember, (void *)0);
  null_task.join();
  if (seen != 0) return 2;

  int *address = &token;
  std::thread typed_task(remember, (void *)address);
  typed_task.join();
  return seen != address;
}
