#include "Socket.h"

#include <stdio.h>
#define BUF_SIZE 256
#define PORT 30000



int main()
{
	UDPSocket socket;

	socket.Open(PORT);


	//// NO BLOCKING MODE
	//DWORD nonBlocking = 1;
	//if (ioctlsocket(handle, FIONBIO, &nonBlocking) != 0)
	//{
	//	printf("Failed to set non-blocking\n");
	//	return(1);
	//}

	
	/*
	
		AcceptedAddress = Socket.Listen();
		Socket.SendAccept(AcceptedAddress);

	*/

	while (TRUE)
	{
		char packet[BUF_SIZE];
		ADRESS sender;

		socket.Receive(sender, packet, BUF_SIZE);

		printf("PACKET FROM(%d.%d.%d.%d:%d)\nMsg:%s\n", sender.a,sender.b, sender.c, sender.d,sender.port, packet);
	}


}
