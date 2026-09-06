int conversions;
template<class T> struct Components {
 T values[4];
 Components(T first, T second, T third, T fourth) {
  values[0]=first; values[1]=second; values[2]=third; values[3]=fourth;
 }
 template<class U> explicit Components(const Components<U>& source) {
  ++conversions;
  for (int i=0; i<4; ++i) values[i] = T(source.values[i]+1);
 }
};
int main() {
 Components<double> source(1.5,2.5,3.5,4.5);
 Components<float> direct(source);
 Components<float> temporary = Components<float>(source);
 if (conversions != 2) return 1;
 for (int i=0; i<4; ++i)
  if (direct.values[i] != source.values[i]+1 || temporary.values[i] != direct.values[i]) return 2;
 return 0;
}
