namespace settings {
int dec(){return 90;}
struct Flags {
  static const int dec=1, skip=8;
  int value;
  Flags():value(dec|skip){}
};
}
int main(){settings::Flags f;return f.value!=9;}
