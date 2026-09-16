#include <regex>
int main(){
  std::regex number("[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?",std::regex_constants::optimize);
  std::cmatch matches;
  const char* text=" -12.5e+2f rest";
  if(!std::regex_search(text,matches,number)||matches.str()!="-12.5e+2f"||matches.position()!=1)return 1;
  if(matches[1].str()!="12.5"||matches[2].str()!=".5"||matches[3].str()!="e+2")return 2;
  if(std::regex_search(text,matches,number,std::regex_constants::match_continuous))return 3;
  if(!std::regex_search(text+1,matches,number,std::regex_constants::match_continuous))return 4;
  std::regex alternatives("a|ab");
  if(!std::regex_match("ab",alternatives)||std::regex_match("abc",alternatives))return 5;
  std::regex copy=number;std::regex moved=std::move(copy);
  if(!std::regex_match(".5",moved))return 6;
  if(!std::regex_match("Hello",std::regex("hello",std::regex::icase)))return 7;
  if(!std::regex_match(L"wide",std::wregex(L"w.de")))return 8;
  bool rejected=false;try{std::regex bad("[");}catch(const std::regex_error&){rejected=true;}
  if(!rejected)return 9;
  return 0;
}
