namespace library {
  class Handle { public: typedef unsigned int id; };
  namespace current {
    inline Handle::id get_id() { return 17; }
  }
}
int main() { return library::current::get_id() != 17; }
