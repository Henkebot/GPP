#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdio.h>

#pragma comment(lib, "Ws2_32.lib")

int main()
{
	// Initialize Winsock
	WSADATA wsData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsData);
	if (iResult != 0)
	{
		printf("WSAStartup failed: %d\n", iResult);
		return 1;
	}

	LPADDRINFO result = NULL;
	LPADDRINFO ptr = NULL;
	ADDRINFOA hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	
	// Resolve the server address and port
#define DEFAULT_PORT "27015"
	iResult = getaddrinfo("192.168.2.8", DEFAULT_PORT, &hints, &result);
	if (iResult != 0)
	{
		printf("getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Attempt to connect to an address until one succeeds
	SOCKET ConnectSocket = INVALID_SOCKET;

	for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
	{
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET)
		{
			printf("Error at socket(): %ld\n", WSAGetLastError());
			freeaddrinfo(result);
			WSACleanup();
			return 1;
		}

		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);

		if (ConnectSocket == INVALID_SOCKET)
		{
			printf("Unable to connect to server! \n");
			WSACleanup();
			return 1;
		}
		break;
	}

	ptr = result;

	// Connect to server.
	

	freeaddrinfo(result);

	
#define DEFAULT_BUFLEN 512
	int recvbuflen = DEFAULT_BUFLEN;
	char* sendbuf = "This is a test";
	char recvbuf[DEFAULT_BUFLEN];

	iResult = send(ConnectSocket, sendbuf, strlen(sendbuf), 0);
	if (iResult == SOCKET_ERROR)
	{
		printf("send failed: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		return 1;
	}

	printf("Bytes Sent %ld\n", iResult);

	// Shutdown the connection for sending since no more data will be sent
	// the client can still use the ConnectSocket for receiving data
	iResult = shutdown(ConnectSocket, SD_SEND);
	if (iResult == SOCKET_ERROR)
	{
		printf("shutdown failed: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		return 1;
	}

	do
	{
		iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0)
		{
			printf("Bytes received: %d\n", iResult);
		}
		else if (iResult == 0)
		{
			printf("Connection closed\n");
		}
		else
		{
			printf("recv failed: %d\n", WSAGetLastError());
		}
	} while (iResult > 0);

	closesocket(ConnectSocket);
	WSACleanup();

	return 0;
}