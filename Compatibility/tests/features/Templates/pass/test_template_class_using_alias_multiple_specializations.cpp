template<typename T>
struct AliasReplayList {
    using iterator = T *;
    using const_iterator = const T *;

    T value;
};

struct OtherAliasReplayValue {
    int value;
};

typedef AliasReplayList<char> CharAliasReplayList;
typedef AliasReplayList<OtherAliasReplayValue> OtherAliasReplayList;

int main()
{
    return 0;
}
