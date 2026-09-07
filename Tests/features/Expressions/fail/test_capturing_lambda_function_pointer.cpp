// EXPECT_COMPILE_FAIL: 1
int main(){int n=3;int(*p)()=[n]{return n;};return p();}
