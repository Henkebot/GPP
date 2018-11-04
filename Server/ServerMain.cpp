#include <WinSock2.h>
#include <WS2tcpip.h> // For ADDRINFO
#include <intsafe.h>
#include <iostream>
#include <MSWSock.h>

const int BUFFER_SIZE = 256;
const char* DEFAULT_PORT = "27015";
const int MAX_WORKER_THREADS = 16;

#define hMalloc(s) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,(s));
#define hFree(s) HeapFree(GetProcessHeap(), 0,(s));

typedef enum _IO_OPERATION
{
	eAccept,
	eRead,
	eWrite
} E_IO_OPERATION;

typedef struct _PER_IO_CONTEXT
{
	WSAOVERLAPPED	Overlapped;
	char			Buffer[BUFFER_SIZE];
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
	void CloseClient(LPPER_SOCKET_CONTEXT _socketContext);

	void ParseAccpetedSocket(SOCKET _socket);
	HANDLE GetIOCPHandle() {
		return mIOCP;
	}
	
private:
	bool mEndServer;
	WSAData mWSAData;
	SOCKET mListenSocket;

private:
	// Threading
	DWORD mThreadCount;
	HANDLE mThreadHandles[MAX_WORKER_THREADS];
	LPPER_SOCKET_CONTEXT UpdateCompletitonPort(SOCKET _socket, E_IO_OPERATION _opt);
	
	// Linked list
	LPPER_SOCKET_CONTEXT mContextList;

	LPPER_SOCKET_CONTEXT 
	CtxtAllocate(SOCKET _socket, E_IO_OPERATION _opt);

	void 
	CtxtListAddTo(LPPER_SOCKET_CONTEXT _context);

	void
	CtxtListDelete(LPPER_SOCKET_CONTEXT _context);

	CRITICAL_SECTION mMutexLock;
	void CreateMutexLock();
	void Lock();
	void Unlock();

private:
	// IOCP
	HANDLE mIOCP;
};

DWORD WINAPI
WorkerThread(LPVOID WorkTheadContext)
{
	IOCPServer* ptr = (IOCPServer*)(WorkTheadContext);
	HANDLE hIOCP = (HANDLE)ptr->GetIOCPHandle();
	BOOL bSuccess = FALSE;
	INT nRet = 0;
	LPWSAOVERLAPPED lpOverlapped = NULL;
	LPPER_SOCKET_CONTEXT lpPerSocketContext = NULL;
	LPPER_IO_CONTEXT	lpIOContext = NULL;
	WSABUF buffRecv;
	WSABUF buffSend;
	DWORD dwRecvNumBytes = 0;
	DWORD dwSendNumBytes = 0;
	DWORD dwFlags = 0;
	DWORD dwIoSize = 0;

	while (TRUE)
	{
		// Continually loop to service io completion packets
		bSuccess = GetQueuedCompletionStatus(hIOCP,
			&dwIoSize,
			(PDWORD_PTR)&lpPerSocketContext,
			(LPOVERLAPPED*)&lpOverlapped,
			INFINITE);
		if (!bSuccess)
			std::cerr << "GetQueuedCompletionStatus() failed: " << GetLastError() << std::endl;
		
		printf("SERVER: THREAD: Service completion packets------------\n");


		if (lpPerSocketContext == NULL)
		{
			//CTRL-C handler used PostQueuedCompletitionStatus to post an I/O packet with
			// a NULL CompletionKey (or if we get on for any reason). It is time to exit.
			return(0);
		}

		//if (g_bEndServer)
		//{
		//	// Main thread will do alll cleanup needed - see finally block
		//	return(0);
		//}

		if (!bSuccess || (bSuccess && (dwIoSize == 0)))
		{
			// Client connection dropped, continue to service remaining ( and possibly
			// new) client connections
			ptr->CloseClient(lpPerSocketContext);
			continue;
		}

		/*
			Determine what type of IO packet has completed by checking PER_IO_CONTEXT
			associated with this socket. This will detemine what action to take.
		*/
		lpIOContext = (LPPER_IO_CONTEXT)lpOverlapped;

		switch (lpIOContext->eIOOperation)
		{
		case eRead:
		{

			printf("SERVER: THREAD: read operation completed\n");

			/*
				a read operation has completed, post a write operation to echo the
				data back to the client using the same data buffer.
			*/
			/*		if (lpPerSocketContext->pCtxtForward)
						lpPerSocketContext->pIOContext->Wsabuf.buf = lpPerSocketContext->pCtxtForward->pIOContext->Buffer;
					else if(lpPerSocketContext->pCtxtBack)
						lpPerSocketContext->pIOContext->Wsabuf.buf = lpPerSocketContext->pCtxtBack->pIOContext->Buffer;*/

			LPPER_IO_CONTEXT lpIOContext = lpPerSocketContext->pIOContext;
			lpIOContext->eIOOperation = eWrite;
			lpIOContext->nTotalBytes = dwIoSize;
			lpIOContext->nSentBytes = 0;
			lpIOContext->WSABuf.len = dwIoSize;
			dwFlags = 0;

			if (lpIOContext->WSABuf.buf[0] == '-')
			{
				sprintf(lpIOContext->WSABuf.buf, "Options!");
			}

			INT nRet = WSASend(lpPerSocketContext->Socket,
				&lpIOContext->WSABuf,
				1,
				&dwSendNumBytes,
				dwFlags,
				&(lpIOContext->Overlapped),
				NULL);

			if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError()))
			{
				std::cerr << "WSASend() failed: " << WSAGetLastError() << std::endl;
				ptr->CloseClient(lpPerSocketContext);
			}
			
				
			
		}
		break;
		case eWrite:
		{
			printf("SERVER: THREAD: write operation completed\n");
			/*
				A write operation has completed, determine if all the data intended to be sent
				actually was sent.
			*/
			lpIOContext->eIOOperation = eWrite;
			lpIOContext->nSentBytes += dwIoSize;
			dwFlags = 0;
			if (lpIOContext->nSentBytes < lpIOContext->nTotalBytes)
			{
				/*
					the previous write operation didn't send all the data,
					post another send to complete the operation
				*/
				buffSend.buf = lpIOContext->Buffer + lpIOContext->nSentBytes;
				buffSend.len = lpIOContext->nTotalBytes - lpIOContext->nSentBytes;
				nRet = WSASend(lpPerSocketContext->Socket,
					&buffSend,
					1,
					&dwSendNumBytes,
					dwFlags,
					&(lpIOContext->Overlapped),
					NULL);
				if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError()))
				{
					std::cerr << "WSASend() failed: " << WSAGetLastError() << std::endl;
					ptr->CloseClient(lpPerSocketContext);
				}
				
				
				printf("SERVER: THREAD: WorkerThread %d: Socket(%d) Send partially completed (%d bytes), Recv posted\n", GetCurrentThreadId(), lpPerSocketContext->Socket,dwIoSize);

				
			}
			else
			{
				// Previous write operation completed for this socket, post another recv
				lpIOContext->eIOOperation = eRead;

				dwRecvNumBytes = 0;
				dwFlags = 0;
				buffRecv.buf = lpIOContext->Buffer;
				buffRecv.len = BUFFER_SIZE;

				nRet = WSARecv(
					lpPerSocketContext->Socket,
					&buffRecv,
					1,
					&dwRecvNumBytes,
					&dwFlags,
					&lpIOContext->Overlapped,
					NULL);

				if (nRet == SOCKET_ERROR && (WSA_IO_PENDING != WSAGetLastError()))
				{
					std::cerr << "WSARecv() failed: " << WSAGetLastError() << std::endl;
					ptr->CloseClient(lpPerSocketContext);
				}
			
				//printf("SERVER: THREAD: recv operation completed\n");
					printf("WorkerThread %d: Socket(%d) Send completed (%d bytes), Recv posted\n",
						GetCurrentThreadId(), lpPerSocketContext->Socket, dwIoSize);
			}
		}
		break;
		}



	}
	return(0);
}

void IOCPServer::Init()
{
	mEndServer = false;
	int nRet;
	nRet = WSAStartup(MAKEWORD(2, 2), &mWSAData);
	if (nRet != 0)
	{
		std::cerr << "TODO" << std::endl;
	}

	mContextList = nullptr;
	CreateMutexLock();

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
	std::clog << "SERVER: bound on ("<< addrlocal->ai_addr->sa_data << ":" << DEFAULT_PORT << ")"<<std::endl;
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
	mThreadCount = 1;
	for (DWORD dwCPU = 0; dwCPU < mThreadCount; dwCPU++)
	{
		DWORD dwThreadID;
		HANDLE hThread = CreateThread(NULL, 0, WorkerThread, (LPVOID)(this), 0, &dwThreadID);
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

	acceptSocket = WSAAccept(mListenSocket, (SOCKADDR*)(&sockAddr), NULL, NULL, 0);

	if (acceptSocket == SOCKET_ERROR)
	{
		std::cerr << "Socket failed to accept " << std::endl;
	}
	std::clog << "Accepted a client" << std::endl;

	return acceptSocket;
}

void IOCPServer::ParseAccpetedSocket(SOCKET _socket)
{
	LPPER_SOCKET_CONTEXT lp = UpdateCompletitonPort(_socket, E_IO_OPERATION::eRead);
	DWORD dwRecvNumBytes = 0;
	DWORD dwFlags = 0;

	// Post inital recv
	int nRet = WSARecv(
		_socket,
		&(lp->pIOContext->WSABuf),
		1,
		&dwRecvNumBytes,
		&dwFlags,
		&(lp->pIOContext->Overlapped),
		nullptr
	);

	if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError()))
	{
		std::cerr << "Failed" << std::endl;
		CloseClient(lp);
	}

}

void IOCPServer::CloseClient(LPPER_SOCKET_CONTEXT _socketContext)
{
	Lock();
	
	closesocket(_socketContext->Socket);
	_socketContext->Socket = INVALID_SOCKET;
	CtxtListDelete(_socketContext);

	Unlock();

}

LPPER_SOCKET_CONTEXT IOCPServer::UpdateCompletitonPort(SOCKET _socket, E_IO_OPERATION _opt)
{
	LPPER_SOCKET_CONTEXT lpPerSocketContext;
	lpPerSocketContext = CtxtAllocate(_socket, _opt);
	if (lpPerSocketContext == nullptr)
	{
		return(nullptr);
	}
	mIOCP = CreateIoCompletionPort((HANDLE)_socket, mIOCP, (DWORD_PTR)lpPerSocketContext, 0);
	if (mIOCP == nullptr)
	{
		std::cerr << "TODO" << std::endl;
		if (lpPerSocketContext->pIOContext)
		{
			hFree(lpPerSocketContext->pIOContext);
		}
		hFree(lpPerSocketContext);
		return(nullptr);
	}
	CtxtListAddTo(lpPerSocketContext);

	return lpPerSocketContext;
}

LPPER_SOCKET_CONTEXT IOCPServer::CtxtAllocate(SOCKET _socket, E_IO_OPERATION _opt)
{
	LPPER_SOCKET_CONTEXT lpPerSocketContext;
	
	Lock();

	lpPerSocketContext = (LPPER_SOCKET_CONTEXT)hMalloc(sizeof(PER_SOCKET_CONTEXT));
	if (lpPerSocketContext)
	{
		lpPerSocketContext->pIOContext = (LPPER_IO_CONTEXT)hMalloc(sizeof(PER_IO_CONTEXT));

		if (lpPerSocketContext->pIOContext)
		{
			lpPerSocketContext->Socket = _socket;
			lpPerSocketContext->pCtxBack = nullptr;
			lpPerSocketContext->pCtxForward = nullptr;

			ZeroMemory(&(lpPerSocketContext->pIOContext->Overlapped), 0);

			lpPerSocketContext->pIOContext->eIOOperation = _opt;
			lpPerSocketContext->pIOContext->pIOContextForward = nullptr;
			lpPerSocketContext->pIOContext->nTotalBytes = 0;
			lpPerSocketContext->pIOContext->nSentBytes = 0;
			lpPerSocketContext->pIOContext->WSABuf.buf = lpPerSocketContext->pIOContext->Buffer;
			lpPerSocketContext->pIOContext->WSABuf.len = sizeof(lpPerSocketContext->pIOContext->Buffer);

			ZeroMemory(lpPerSocketContext->pIOContext->WSABuf.buf, lpPerSocketContext->pIOContext->WSABuf.len);

		}
		else
		{
			hFree(lpPerSocketContext);
		}
	}
	else
	{
		std::cerr << "SERVER: Failed to heap alloc Socket context" << std::endl;
	}

	Unlock();
	return lpPerSocketContext;
}

void IOCPServer::CtxtListAddTo(LPPER_SOCKET_CONTEXT _context)
{
	
	Lock();

	if (mContextList == nullptr)
	{
		_context->pCtxBack = nullptr;
		_context->pCtxForward = nullptr;
		mContextList = _context;
	}
	else
	{
		LPPER_SOCKET_CONTEXT pTemp;

		pTemp = mContextList;
		mContextList = _context;

		_context->pCtxBack = pTemp;
		_context->pCtxForward = nullptr;

		pTemp->pCtxForward = _context;

	}

	Unlock();


}

void IOCPServer::CtxtListDelete(LPPER_SOCKET_CONTEXT _context)
{
	Lock();

	LPPER_SOCKET_CONTEXT pBack;
	LPPER_SOCKET_CONTEXT pForward;
	LPPER_IO_CONTEXT PTempIO;
	
	pBack = _context->pCtxBack;
	pForward = _context->pCtxForward;

	if (pBack == nullptr && pForward == nullptr)
	{
		// This is the only node in the list to delete
		mContextList = nullptr;
	}
	else if (pBack == nullptr && pForward != nullptr)
	{
		// This is the start node in the list to delete
		pForward->pCtxBack = nullptr;
		mContextList = pForward;
	}
	else if (pBack == nullptr && pForward != nullptr)
	{
		pBack->pCtxForward = nullptr;
	}
	else if (pBack && pForward)
	{
		pBack->pCtxForward = pForward;
		pForward->pCtxBack = pBack;
	}

	PTempIO = (LPPER_IO_CONTEXT)(_context->pIOContext);
	LPPER_IO_CONTEXT pNextIO;
	do
	{
		pNextIO = (LPPER_IO_CONTEXT)(PTempIO->pIOContextForward);
		if (pNextIO)
		{
			if (mEndServer)
			{
				while (!HasOverlappedIoCompleted((LPOVERLAPPED)PTempIO)) Sleep(0);
			}
			hFree(PTempIO);
			PTempIO = nullptr;
		}
		PTempIO = pNextIO;
	} while (pNextIO);
	hFree(_context);
	_context = nullptr;

	Unlock();

}

void IOCPServer::CreateMutexLock()
{
	__try
	{
		InitializeCriticalSection(&mMutexLock);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		std::cerr << "TODO" << std::endl;
	}

}

void IOCPServer::Lock()
{
	__try
	{
		EnterCriticalSection(&mMutexLock);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		std::cerr << "TODO" << std::endl;
	}
}

void IOCPServer::Unlock()
{
	LeaveCriticalSection(&mMutexLock);
}

int main()
{
	IOCPServer server;

	server.Init();
	server.SpawnWorkerThreads();
	server.CreateListenSocket();

	while (TRUE)
	{
		SOCKET s = server.Accept();

		server.ParseAccpetedSocket(s);


	}
}