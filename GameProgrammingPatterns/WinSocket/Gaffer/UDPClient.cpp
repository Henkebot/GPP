#include "Socket.h"
#include <stdio.h>

#define BUF_SIZE 256
#define PORT 30000



int main()
{

	UDPSocket s;
	ADRESS addr;
	addr.a = 127;
	addr.b = 0;
	addr.c = 0;
	addr.d = 1;
	addr.port = PORT;
	const char buffer[BUF_SIZE] = { 'H','E','L','L','O','\0' };
	INT i = 0;
	while(i++ != 10)
		s.Send(addr, buffer, BUF_SIZE);

	
	return(0);

}
