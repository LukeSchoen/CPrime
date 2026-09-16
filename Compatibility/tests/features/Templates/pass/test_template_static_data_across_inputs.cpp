// EXPECT_SOURCES: ["template_static_data_linkage_other.cpp"]
#include "template_static_data_linkage.h"
int main(){int* p=other_store();if(p!=&SharedStore<int>::value||*p!=5)return 1;SharedStore<int>::value=11;return *p!=11;}
