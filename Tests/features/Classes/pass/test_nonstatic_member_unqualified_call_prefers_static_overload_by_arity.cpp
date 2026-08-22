class PathLike
{
public:
  PathLike() = default;
  PathLike(const PathLike &other) {}

  PathLike ResolveRelativePath(const PathLike &target) const;
  static PathLike ResolveRelativePath(const PathLike &source, const PathLike &target);
};

PathLike PathLike::ResolveRelativePath(const PathLike &source, const PathLike &target)
{
  PathLike ret;
  return ret;
}

PathLike PathLike::ResolveRelativePath(const PathLike &target) const
{
  PathLike source;
  return ResolveRelativePath(source, target);
}

int main()
{
  return 0;
}
