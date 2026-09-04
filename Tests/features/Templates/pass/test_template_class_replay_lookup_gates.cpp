namespace replay_lookup_namespace {
struct Marker {
    int value;
};
}

template<typename U>
struct ReplayNested {
    U value;
};

template<typename T>
struct ReplayLookupGate {
    using Value = T;

    ReplayNested<T> nested;
    replay_lookup_namespace::Marker marker;
    Value value;

    int Read(int ReplayNested) const
    {
        return ReplayNested + nested.value + marker.value + value;
    }
};

int main()
{
    ReplayLookupGate<int> first;
    ReplayLookupGate<char> second;
    first.nested.value = 2;
    first.marker.value = 3;
    first.value = 4;
    second.nested.value = 1;
    second.marker.value = 1;
    second.value = 1;
    return first.Read(1) == 10 && second.Read(1) == 4 ? 0 : 1;
}
