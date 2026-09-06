#include "template_static_data_linkage.h"
int* other_store(){return &SharedStore<int>::value;}
