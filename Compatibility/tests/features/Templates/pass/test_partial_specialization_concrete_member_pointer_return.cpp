// EXPECT_COMPILE_ONLY: 1
template<class> struct Dispatch;

template<class Owner>
struct Dispatch<void (Owner::*)(int&)>
{
  struct Entry {};
  template<class Handler>
  void attach(void (Handler::*)(int&)) {}
};

struct Reader { void run(int&); };

template<class Owner>
struct Derived : Dispatch<void (Owner::*)(int&)>
{
  void connect() { this->attach(&Owner::run); }
};

template<>
void Derived<Reader>::connect() { this->attach(&Reader::run); }
