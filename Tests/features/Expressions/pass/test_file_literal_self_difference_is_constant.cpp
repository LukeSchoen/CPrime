/* Equal literal occurrences share emitted storage, so their relocation bases
   cancel during constant evaluation as well as in runtime pointer arithmetic. */
enum {
  file_difference = __FILE__ - __FILE__,
  builtin_file_difference = __builtin_FILE () - __builtin_FILE (),
  function_difference = __builtin_FUNCTION () - __builtin_FUNCTION ()
};

int main () {
  return file_difference || builtin_file_difference || function_difference;
}
