#include "gc.h"

void test()
{
	GCBEGIN
		for (int i = 1; i <= 5; i++)
		{
			GCArenaAlloc(arena, i * i);
		}
	GCEND
}

int main(int argc, char const *argv[])
{
	InitializeGC();
	test();
	DisposeGC();
	return 0;
}