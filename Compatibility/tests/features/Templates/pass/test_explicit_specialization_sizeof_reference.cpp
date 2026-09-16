typedef char (&bidirectional_t)[2];
typedef char (&random_access_t)[3];

static_assert(sizeof(bidirectional_t) == 2, "reference to array[2]");
static_assert(sizeof(random_access_t) == 3, "reference to array[3]");

template<unsigned S>
struct impl
{
};

template<>
struct impl<sizeof(bidirectional_t)>
{
};

template<>
struct impl<sizeof(random_access_t)>
{
};

int main()
{
	return 0;
}
