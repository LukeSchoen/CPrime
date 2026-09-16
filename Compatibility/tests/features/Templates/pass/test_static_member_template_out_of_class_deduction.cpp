struct Writer {
 template<class Value> static int count(int tag, const Value &value);
 template<class Value> static int count(int tag, const Value *value, int size);
};
template<class Value> int Writer::count(int tag, const Value &value) { return count(tag, &value, 1); }
template<class Value> int Writer::count(int tag, const Value *value, int size) { return tag + (int)sizeof(Value) * size; }
int main() { char c=0; if(Writer::count(7,c)!=8)return 1; double v[2]={}; return Writer::count(3,v,2)!=19; }
