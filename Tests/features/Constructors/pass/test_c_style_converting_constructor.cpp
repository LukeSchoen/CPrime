struct Text {long long value;template<class T> explicit Text(T n):value((long long)n){} };
int main(){Text t=(Text)(9LL);return t.value!=9;}
