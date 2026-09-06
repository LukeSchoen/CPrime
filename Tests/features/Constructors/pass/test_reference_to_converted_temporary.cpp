struct Text {
  static int alive;
  const char* text;
  Text(const char* s):text(s){++alive;}
  ~Text(){--alive;}
};
int Text::alive=0;
int main(){
  { const Text& value="abc"; if(Text::alive!=1 || value.text[1]!='b')return 1; }
  return Text::alive!=0;
}
