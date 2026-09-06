struct Text {
  const char *data;
  Text(const char *value) : data(value) {}
};
enum Kind : long long { Texture = 2 };
struct Owner {
  struct Resource {
    Text name;
    Kind kind;
    Resource(Text text, Kind value);
  };
};
Owner::Resource::Resource(Text text, Kind value) : name(text), kind(value) {}
struct Pair {
  Text first, second;
  Pair(Text a, Text b) : first(a), second(b) {}
};
int main() {
  Owner::Resource resource("texture", Texture);
  Pair pair("one", "two");
  return resource.name.data[0] != 't' || resource.kind != Texture
      || pair.first.data[0] != 'o' || pair.second.data[0] != 't';
}
