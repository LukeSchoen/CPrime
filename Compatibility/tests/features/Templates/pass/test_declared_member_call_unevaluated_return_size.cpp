// EXPECT_EXIT: 0
struct Probe { template<class T> static T read(T*); };
int main() {
    return sizeof(Probe::read((char*)0)) != sizeof(char)
        || sizeof(Probe::read((long long*)0)) != sizeof(long long)
        || sizeof(Probe::read((char*)0)) != sizeof(char);
}
