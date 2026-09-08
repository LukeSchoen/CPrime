template<int N> struct Target { int value; };
template<int N> struct Source {
    operator Target<3>() { Target<3> result = {N}; return result; }
};
int main() {
    Source<7> source;
    Target<3> target = source;
    return target.value != 7;
}
