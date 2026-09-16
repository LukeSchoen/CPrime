#define STRINGIZE_INNER(x) #x
#define STRINGIZE(x) STRINGIZE_INNER(x)
#define BOOST_MPL_PREPROCESSED_HEADER or.hpp

int main()
{
	const char *header = STRINGIZE(BOOST_MPL_PREPROCESSED_HEADER);
	return header[0] == 'o' && header[1] == 'r' && header[2] == '.' ? 0 : 1;
}
