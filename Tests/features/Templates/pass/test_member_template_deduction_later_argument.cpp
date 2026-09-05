struct Output { int value; };
template<class Tag> struct Worker {
  template<class Result> static void set(int value, Result &output) { output.value = value; }
  template<class Result> void add(int value, Result &output) { output.value += value; }
};
int main() {
  Output output = {0};
  Worker<char>::set(7, output);
  if (output.value != 7) return 1;
  Worker<char> worker;
  worker.add(11, output);
  return output.value != 18;
}
