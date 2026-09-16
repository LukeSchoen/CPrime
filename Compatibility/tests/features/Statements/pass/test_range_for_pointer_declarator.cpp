int main(){int a=1,b=2;int* values[]={&a,&b};for(auto* p:values)*p+=2;for(auto* const& p:values)*p+=1;for(const auto* p:values)if(*p<4)return 1;return a!=4 || b!=5;}
