int* pointer(){return {};}
int number(){return {7};}
double zero(){return {};}
int main(){return pointer()!=0||number()!=7||zero()!=0.0;}
