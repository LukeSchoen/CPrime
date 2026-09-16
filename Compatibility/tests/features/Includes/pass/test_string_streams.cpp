#include <sstream>
#include <iomanip>
int main(){
  std::stringstream s;
  std::ostream& out=s; std::istream& in=s;
  out << std::hex << std::setfill('0') << std::setw(4) << 42 << ' ' << std::dec << 17;
  if(s.str()!="002a 17")return 1;
  if(out.tellp()!=7)return 2;
  char data[16]={0};in.read(data,4);
  if(std::string(data,4)!="002a"||in.tellg()!=4)return 3;
  out.seekp(0);out << "AB";
  if(s.str()!="AB2a 17")return 4;
  in.seekg(0);in.read(data,16);
  if(in.gcount()!=7||!in.eof()||!s.fail())return 5;
  s.clear();s.str("");out << std::fixed << std::setprecision(2) << 1.25 << ' ' << false << ' ' << std::boolalpha << true;
  if(s.str()!="1.25 0 true")return 6;
  std::ostringstream append(std::string("start"),std::ios::app);append << "+";
  if(append.str()!="start+")return 7;
  std::istringstream binary(std::string("a\0b",3));binary.read(data,3);
  if(binary.gcount()!=3||data[1]!=0||data[2]!='b')return 8;
  std::wstringstream wide;wide << L"wide" << 42;
  return wide.str()!=L"wide42";
}
