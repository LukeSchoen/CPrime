template<class T0, class T1, class T2, class T3, class T4,
         class T5, class T6, class T7, class T8, class T9,
         class T10, class T11, class T12, class T13, class T14,
         class T15, class T16, class T17, class T18, class T19>
struct Many
{
	static const int count = 20;
};

int main()
{
	return Many<int, int, int, int, int, int, int, int, int, int,
	            int, int, int, int, int, int, int, int, int, int>::count == 20 ? 0 : 1;
}
