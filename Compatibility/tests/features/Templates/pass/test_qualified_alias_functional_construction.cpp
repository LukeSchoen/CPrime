#include <map>
#include <string>
struct Editor {
  typedef std::map<int,std::string> Errors;
  Errors errors;
  void add() { errors.insert(Errors::value_type(7,"error")); }
};
int main(){Editor e;e.add();auto v=Editor::Errors::value_type(8,"next");return e.errors[7]!="error"||v.first!=8||v.second!="next";}
