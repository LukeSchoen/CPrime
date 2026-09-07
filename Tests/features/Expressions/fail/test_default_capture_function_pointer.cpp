// EXPECT_COMPILE_FAIL: 1
int main(){int(*p)()=[=]{return 3;};return p();}
