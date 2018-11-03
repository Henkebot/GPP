#include "Socket.h"
#include <stdio.h>
UDPSocket::UDPSocket()
{
	m_Socket = INVALID_SOCKET;
	WSADATA wsaData;
	//No Error checking !
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) == NO_ERROR)
	{
		m_Socket = socket(AF_INET,
			SOCK_DGRAM,
			IPPROTO_UDP);
	}


}

UDPSocket::~UDPSocket()
{
	Close();
}

BOOL UDPSocket::Open(unsigned short port)
{
	SOCKADDR_IN adress;
	adress.sin_family = AF_INET;
	adress.sin_addr.s_addr = INADDR_ANY;
	adress.sin_port = htons(port);

	if (bind(m_Socket, (const SOCKADDR*)&adress, sizeof(SOCKADDR_IN)) < 0)
	{
		return(0);
	}
	return(1);
}

VOID UDPSocket::Close()
{
	WSACleanup();
	m_Socket = INVALID_SOCKET;
}

ADRESS UDPSocket::Listen()
{
	ADRESS sender;

	return sender;
}

BOOL UDPSocket::Connect(const ADRESS & _destination)
{
	

	return 0;
}

BOOL UDPSocket::isOpen()
{
	// Dont know if this is correct
	return m_Socket != INVALID_SOCKET;
}

BOOL UDPSocket::Send(const ADRESS & _destination, const CHAR * _lpBuffer, INT nSize)
{
	//Apply header
	CHAR finalBuffer[256 + 256] = "LOL";
	sprintf(finalBuffer, "%s%s", finalBuffer, _lpBuffer);

	SOCKADDR_IN addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(_destination.address);

	addr.sin_port = htons(_destination.port);

	int nSentBytes = sendto(m_Socket, finalBuffer, nSize + 256, 0, (SOCKADDR*)&addr, sizeof(SOCKADDR_IN));

	return(nSentBytes == nSize);
}

INT UDPSocket::Receive(ADRESS & _sender, CHAR* _lpBuffer, INT _nBufferSize)
{
	SOCKADDR_IN from;
	int fromLength = sizeof(from);
	CHAR recvBuffer[512];
	int nBytes = recvfrom(m_Socket, recvBuffer, _nBufferSize + 256, 0, (SOCKADDR*)&from, &fromLength);
	if (nBytes > 0)
	{
		char headerCheck[] = "LOL\0";
		for (UINT8 i = 0; i < 3; i++)
		{
			if (recvBuffer[i] != headerCheck[i]) // Failed the header check
				return(0);
		}
		
		sprintf(_lpBuffer, "%s", recvBuffer + 256);
	}
	_sender.address = ntohl(from.sin_addr.s_addr);
	_sender.port  = ntohs(from.sin_port);
	return nBytes;
}
