#pragma once
#include <winsock2.h>
#pragma comment(lib, "Ws2_32.lib")

typedef struct _ADDRESS
{
	union
	{
		struct
		{
			UINT8 d, c, b, a;
		};

		UINT32 address;
	};
	UINT16 port;
}ADRESS, *LPADDRESS;

class UDPSocket
{
private:
	SOCKET m_Socket;
public:
	UDPSocket();

	~UDPSocket();

	BOOL 
	Open(unsigned short port);

	VOID
	Close();

	ADRESS
	Listen();

	BOOL
	Connect(const ADRESS& _destination);

	BOOL
	isOpen();

	BOOL
	Send(const ADRESS& _destination,
		const CHAR* _lpBuffer,
		INT nSize);

	INT
	Receive(ADRESS& _sender, 
		CHAR* _lpBuffer, 
		INT _nBufferSize);
	


};
