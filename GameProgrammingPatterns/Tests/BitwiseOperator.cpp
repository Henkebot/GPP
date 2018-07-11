#include <stdio.h>
#include <stdint.h>

unsigned int lool = UINT16_C(100);

#define A (1 << 0) // 00000001
#define B (1 << 1) // 00000010

int gameControllerStatus = 0;


void keyPressed(int key)
{
	gameControllerStatus |= key;
}

void keyRelased(int key)
{
	gameControllerStatus &= ~key;
}

bool isKeyPressed(int key)
{
	return gameControllerStatus & key;
}


struct Test
{
	unsigned int k : 1;
	unsigned int y : 1;
	unsigned int z : 2;
	unsigned int u : 4;
};

int main()
{
	Test lol;
	printf("Sizeof Test = %i", sizeof(lol));

	int value = -20;
	
	int isNeg = (value & (1 << 31));

	isNeg = (value  < 0);

	int y = 20;

	/*
	keyPressed(A);
	printf("Is A pressed %i\n", isKeyPressed(A));
	keyRelased(A);
	printf("Is A pressed %i\n", isKeyPressed(A));

	int y = 0xfffffff - 1;

	printf("Highest number is %i", y);
	*/


	return 0;
}