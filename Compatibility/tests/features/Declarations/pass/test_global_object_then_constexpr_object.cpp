struct Global
{
	Global();
};

Global global;

Global::Global()
{
}

struct Probe
{
	explicit constexpr Probe(int) {}
};

constexpr Probe probe(0);

int main()
{
	return 0;
}
