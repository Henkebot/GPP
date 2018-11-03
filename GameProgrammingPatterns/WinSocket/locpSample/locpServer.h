#pragma once


#include <MSWSock.h>
#define DEFAULT_PORT "27015"
#define MAX_BUF_SIZE 8192
#define MAX_WORKER_THREAD 16
#pragma comment(lib, "Ws2_32.lib")

typedef enum _IO_OPERATION
{
	ClientIoAccept,
	ClientIoRead,
	ClientIoWrite
} IO_OPERATION, *LPIO_OPERATION;

// Data to be associated for every I/O operation on a socket

typedef struct _PER_IO_CONTEXT
{
	WSAOVERLAPPED		Overlapped;
	char				Buffer[MAX_BUF_SIZE];
	WSABUF				Wsabuf;
	int					nTotalBytes;
	int					nSentBytes;
	IO_OPERATION		IOOperation;
	SOCKET				SocketAccept;

	_PER_IO_CONTEXT		*pIOContextForward;
} PER_IO_CONTEXT, *LPPER_IO_CONTEXT;


typedef struct _PER_SOCKET_CONTEXT
{
	SOCKET			Socket;
	LPFN_ACCEPTEX	fnAcceptEx;

	// Linked list for all outstanding i/o on the socket

	LPPER_IO_CONTEXT		pIOContext;
	_PER_SOCKET_CONTEXT		*pCtxtBack;
	_PER_SOCKET_CONTEXT		*pCtxtForward;
} PER_SOCKET_CONTEXT, *LPPER_SOCKET_CONTEXT;

BOOL 
ValidOptions (int argc, char* argv[]);

BOOL WINAPI
CtrlHandler(DWORD dwEvent);

BOOL
CreateListenSocket(void);

DWORD WINAPI
WorkerThread(LPVOID WorkContext);

void Send(const LPPER_IO_CONTEXT &lpIOContext, const DWORD &dwIoSize, DWORD &dwFlags, INT &nRet, const LPPER_SOCKET_CONTEXT &lpPerSocketContext, DWORD &dwSendNumBytes);

LPPER_SOCKET_CONTEXT
UpdateCompletionPort(
	SOCKET s,
	IO_OPERATION,
	BOOL bAddToList
);

VOID 
CloseClient(
	LPPER_SOCKET_CONTEXT lpPerSocketContext,
	BOOL bGraceful
);

LPPER_SOCKET_CONTEXT
CtxtAllocate(
	SOCKET s,
	IO_OPERATION ClientIo
);

VOID
CtxtListFree();

VOID
CtxtListAddTo(
	LPPER_SOCKET_CONTEXT lpPerSocketContext
);

VOID
CtxtListDeleteFrom(
	LPPER_SOCKET_CONTEXT lpPerSocketContext
);