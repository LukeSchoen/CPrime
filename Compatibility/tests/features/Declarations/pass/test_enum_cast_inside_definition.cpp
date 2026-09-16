// An enumerator may cast an integral value back to the enum being defined.
enum GapColour
{
	GapRed = (GapColour)(0x00010000 | 1),
	GapGreen = (GapColour)(0x00010000 | 2)
};

int main()
{
	return (int)GapRed == 0x00010001 && (int)GapGreen == 0x00010002 ? 0 : 1;
}
