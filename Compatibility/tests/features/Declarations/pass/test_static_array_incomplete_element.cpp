struct Element;
struct Table { static Element values[]; };
struct Element { int value; };
Element Table::values[]={{7},{9}};
int main(){return Table::values[1].value==9?0:1;}
