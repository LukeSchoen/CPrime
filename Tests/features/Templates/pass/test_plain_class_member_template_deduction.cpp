struct Output { int value; };
struct Worker {
 template<class Result> static void set(int value, Result &output) { output.value=value; }
 template<class Result> void add(int value, Result &output) { output.value+=value; }
};
int main(){ Output output={0}; Worker::set(7,output); Worker worker; worker.add(11,output); return output.value!=18; }
