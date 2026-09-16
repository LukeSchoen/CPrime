struct Text {int value;Text(const char*):value(7){} };
int check(const char* text){return text[0]=='R'?3:1;}
int check(const Text& text){return text.value;}
struct Log {
 template<class... A> int record(const Text&,A&&... a){int total=0;int unused[]={((total+=check(a)),0)...};return total;}
};
int main(){Log log;return log.record("event","Reason NoAcceptableZone",Text("pos"),Text("x"))!=17;}
