#include <fstream>
#include <stdio.h>
int main(){
 const char* path="cprime_fstream_regression.tmp";
 {std::ofstream file(path,std::ios::binary|std::ios::trunc);if(!file.is_open()||!file.good())return 1;file.write("a\0b",3);file.close();if(file.is_open())return 2;}
 {std::ofstream file(path,std::ios::app|std::ios::binary);file<<"c";}
 {std::ifstream file(path,std::ios::binary|std::ios::ate);if(file.tellg()!=4)return 3;file.seekg(0);char value[5]={};file.read(value,5);if(file.gcount()!=4||!file.eof()||value[0]!='a'||value[1]!=0||value[2]!='b'||value[3]!='c')return 4;file.clear();file.seekg(0);if(!file.good())return 5;}
 remove(path);
 std::ifstream absent(path);if(absent.good()||absent.is_open())return 6;
 std::ofstream closed;closed<<7;if(!closed.bad())return 8;closed.clear();closed<<"no file";if(!closed.bad())return 9;closed.close();if(!closed.fail())return 7;
 return 0;
}
