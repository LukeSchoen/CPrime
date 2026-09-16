template<class F>bool invoke(F f){return f();}
struct Semaphore {
 int count;
 bool ready(){return invoke([this]{return count>0;});}
 bool reference(){return invoke([&]{return this->count>0;});}
 bool value(){return invoke([=]{return this->count>0;});}
 bool method(){return invoke([=]{return ready();});}
 bool nested(){return invoke([this]{return invoke([this]{return this->count>0;});});}
};
int main(){Semaphore s={1};return !s.ready()||!s.reference()||!s.value()||!s.nested()||!s.method();}
