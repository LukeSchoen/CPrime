#include <utility>
#include <string>
#include <vector>
enum class Kind {Word};
int main(){std::vector<std::pair<std::string,Kind>> values;values.push_back(std::make_pair<std::string,Kind>("word",Kind::Word));return values[0].first!="word"||values[0].second!=Kind::Word;}
