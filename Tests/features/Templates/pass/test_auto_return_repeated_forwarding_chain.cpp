template<class T> auto level0(T v){return v+0.5;}
template<class T> auto level1(T v){return level0(v)+level0(v);}
template<class T> auto level2(T v){return level1(v)+level1(v);}
template<class T> auto level3(T v){return level2(v)+level2(v);}
template<class T> auto level4(T v){return level3(v)+level3(v);}
template<class T> auto level5(T v){return level4(v)+level4(v);}
template<class T> auto level6(T v){return level5(v)+level5(v);}
template<class T> auto level7(T v){return level6(v)+level6(v);}
template<class T> auto level8(T v){return level7(v)+level7(v);}
template<class T> auto level9(T v){return level8(v)+level8(v);}
template<class T> auto level10(T v){return level9(v)+level9(v);}
template<class T> auto level11(T v){return level10(v)+level10(v);}
template<class T> auto level12(T v){return level11(v)+level11(v);}
template<class T> auto level13(T v){return level12(v)+level12(v);}
int main(){return level13(1)!=12288.0;}
