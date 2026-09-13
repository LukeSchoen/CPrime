typedef struct { double value; } *RecordPointer;

int accept_record_pointer(RecordPointer pointer)
{
  return pointer == 0 ? 4 : 1;
}

template<class T> struct Box {
  static int size() { return sizeof(T); }
};

static int local_template_argument_size()
{
  struct LocalRecord { };
  return Box<LocalRecord>::size();
}

int main()
{
  return accept_record_pointer(0) != 4 || local_template_argument_size() != 1;
}
