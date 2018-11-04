#include <WinSock2.h>
#include <WS2tcpip.h> // For ADDRINFO
#include <intsafe.h>
#include <iostream>
#include <MSWSock.h>

const BYTE BUFFER_SIZE = 256;
const char* DEFAULT_PORT = "27015";
const int MAX_WORKER_THREADS = 16;

#define hMalloc(s) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,(s));
#define hFree(s) HeapFree(GetProcessHeap(), 0,(p));

typedef enum _IO_OPERATION
{
	eAccept,
	eRead,
	eWrite
} E_IO_OPERATION;

typedef struct _PER_IO_CONTEXT
{
	WSAOVERLAPPED	Overlapped;
	BYTE			buffer[BUFFER_SIZE];
	WSABUF			WSABuf;
	int				nTotalBytes;
	int				nSentBytes;
	E_IO_OPERATION	eIOOperation;
	SOCKET			SocketAccept;

	_PER_IO_CONTEXT *pIOContextForward;

} PER_IO_CONTEXT, *LPPER_IO_CONTEXT;

typedef struct _PER_SOCKET_CONTEXT
{
	SOCKET Socket;
	LPFN_ACCEPTEX fnAcceptEx;

	// Linked list for all the outstanding i/o on the socket
	LPPER_IO_CONTEXT pIOContext;
	_PER_SOCKET_CONTEXT *pCtxBack;
	_PER_SOCKET_CONTEXT *pCtxForward;

} PER_SOCKET_CONTEXT, *LPPER_SOCKET_CONTEXT;

class IOCPServer
{
public:
	void Init(void);
	void CreateListenSocket(void);
	void SpawnWorkerThreads(void);
	SOCKET Accept();

	void ParseAccpetedSocket(SOCKET _socket);
	
private:
	WSAData mWSAData;
	SOCKET mListenSocket;

private:
	// Threading
	DWORD mThreadCount;
	HANDLE mThreadHandles[MAX_WORKER_THREADS];
	LPPER_SOCKET_CONTEXT UpdateCompletitonPort(SOCKET _socket, E_IO_OPERATION _opt);
	
	CRITICAL_SECTION mMutexLock;

private:
	// IOCP
	HANDLE mIOCP;
};

DWORD WINAPI
WorkerThread(LPVOID WorkTheadContext)
{

}

void IOCPServer::Init()
{
	int nRet;
	nRet = WSAStartup(MAKEWORD(2, 2), &mWSAData);
	if (nRet != 0)
	{
		std::cerr << "TODO" << std::endl;
	}

}

void IOCPServer::CreateListenSocket(void)
{
	int nRet; // Used to check return values;
	ADDRINFO hints;
	LPADDRINFO addrlocal = nullptr;

	ZeroMemory(&hints, sizeof(hints));
	// Resolve the interface
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM; // TCP
	hints.ai_protocol = IPPROTO_IP;

	nRet = getaddrinfo(NULL, DEFAULT_PORT, &hints, &addrlocal);

	if (nRet != 0)
	{
		std::cerr << "TODO" << std::endl;
		return;
	}

	// Create socket
	mListenSocket = WSASocketW(addrlocal->ai_family,
		addrlocal->ai_socktype,
		addrlocal->ai_protocol,
		NULL,
		0,
		WSA_FLAG_OVERLAPPED
	);

	if (mListenSocket == INVALID_SOCKET)
	{
		std::cerr << "TODO" << std::endl;
		return;
	}

	// Bind the socket
	nRet = bind(mListenSocket, addrlocal->ai_addr, static_cast<int>(addrlocal->ai_addrlen));

	if (nRet == SOCKET_ERROR)
	{
		std::cerr << "TODO" << std::endl;
		return;
	}

	nRet = listen(mListenSocket, 5);
	
	if (nRet == SOCKET_ERROR)
	{
		std::cerr << "TODO" << std::endl;
		return;
	}

	// Taken from MSDN, I dont know the gain from doing this
	/*
	Disable send buffering on the socket. Setting SO_SNDBUF
	to 0 causes winsock to stop buffering sends and perform
	sends directly from our buffers, thereby reducing CPU usage.

	However, this does prevent the socket from ever filling the
	send pipeline. This can lead to packets being sent that are
	not full (i.e. the overhead of the IP and TCP headers is
	great compared to the amount of data being carried).

	Disabling the send buffer has less serious repercussions
	than disabling the receive buffer.
	*/
	int nZero = 0;
	nRet = setsockopt(mListenSocket, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<char*>(&nZero), sizeof(nZero));

	if (nRet == SOCKET_ERROR)
	{
		std::cerr << "TODO" << std::endl;
		return;
	}

	freeaddrinfo(addrlocal);





}

void IOCPServer::SpawnWorkerThreads(void)
{

	// Create IOCP Handle
	mIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (mIOCP == nullptr)
	{
		std::cerr << "TODO" << std::endl;
	}

	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	mThreadCount = static_cast<int>(sysInfo.dwNumberOfProcessors) * 2;

	for (DWORD dwCPU = 0; dwCPU < mThreadCount; dwCPU++)
	{
		DWORD dwThreadID;
		HANDLE hThread = CreateThread(NULL, 0, WorkerThread, mIOCP, 0, &dwThreadID);
		if (hThread == NULL)
		{
			std::cerr << "TODO" << std::endl;
		}

		mThreadHandles[dwCPU] = hThread;
	}
}

SOCKET IOCPServer::Accept()
{
	SOCKADDR_IN sockAddr;
	SOCKET acceptSocket;

	acceptSocket = WSAAccept(mListenSocket, reinterpret_cast<SOCKADDR*>(&sockAddr), NULL, NULL, 0);

	getsockname(acceptSocket, reinterpret_cast<SOCKADDR*>(&sockAddr), reinterpret_cast<int*>(sizeof(sockAddr)));
	std::cout << "SERVER: Accepted Connection from: " << inet_ntoa(sockAddr.sin_addr);

	return acceptSocket;
}

void IOCPServer::ParseAccpetedSocket(SOCKET _socket)
{
	LPPER_SOCKET_CONTEXT lp = UpdateCompletitonPort(_socket, E_IO_OPERATION::eRead);
}

LPPER_SOCKET_CONTEXT IOCPServer::UpdateCompletitonPort(SOCKET _socket, E_IO_OPERATION _opt)
{
	LPPER_SOCKET_CONTEXT lpPerSocketContext;
	lpPerSocketContext = 
	return LPPER_SOCKET_CONTEXT();
}

int main()
{
	IOCPServer server;

	server.Init();
	server.CreateListenSocket();
	server.SpawnWorkerThreads();

	while (TRUE)
	{
		SOCKET s = server.Accept();

	}
}