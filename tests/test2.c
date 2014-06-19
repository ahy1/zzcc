
int main(int /*@unused@*/argc, char /*@unused@*/*argv[])
{
	int i, j;

	i=0;
	j=0;

	while (i<10) {
		j+=++i;
	}

	return i+j;
}

