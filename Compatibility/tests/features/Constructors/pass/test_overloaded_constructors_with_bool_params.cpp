// EXPECT_EXIT: 0
class Audio
{
public:
  int mode;
  Audio(const char *filename, float volume, bool repeat);
  Audio(const char *filename, float volume, bool repeat, bool playNow);
};

Audio::Audio(const char *filename, float volume, bool repeat)
{
  mode = filename[0] == 'a' && volume > 0.5f && repeat ? 3 : 0;
}

Audio::Audio(const char *filename, float volume, bool repeat, bool playNow)
{
  mode = filename[0] == 'b' && volume > 0.5f && repeat && playNow ? 4 : 0;
}

int main()
{
  Audio a("a", 1.0f, true);
  Audio b("b", 1.0f, true, true);
  return a.mode == 3 && b.mode == 4 ? 0 : 1;
}
