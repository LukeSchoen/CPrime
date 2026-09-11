/* GCC merges equal string constants, so a file-name literal subtracted from
   itself is a constant zero: `__FILE__` and `__builtin_FILE ()` denote one
   object.  Building a separate literal per spelling leaves a relocation that
   the constant evaluator rejects. */
enum {
  file_difference = __FILE__ - __FILE__,
  builtin_file_difference = __builtin_FILE () - __builtin_FILE (),
  function_difference = __builtin_FUNCTION () - __builtin_FUNCTION ()
};

int main () {
  return file_difference || builtin_file_difference || function_difference;
}
