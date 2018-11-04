#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif // !WIN32_LEAN_AND_MEAN


#include <WinSock2.h>
#include <WS2tcpip.h>
#include <strsafe.h> // for StringCchVPrintf
#include <stdio.h>
#include <stdlib.h>

#include "locpServer.h"
#define xmalloc(s) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,(s));
#define xfree(p) HeapFree(GetProcessHeap(),0,(p));



#define global static

global DWORD g_dwThreadCount = 0; // Worker thread count
global HANDLE g_ThreadHandles[MAX_WORKER_THREAD];
// Linked list of context info structures
// maintained to allow the cleanup handler 
// to cleanly close all sockets and free resources.
global LPPER_SOCKET_CONTEXT g_pCtxList = NULL; 

global CRITICAL_SECTION g_CriticalSection;		// Guard access to the global context list ( Mutex lock )

global BOOL g_bVerbose = TRUE;
global BOOL g_bRestart = TRUE;		// set to TRUE on CTRL-BRK
global BOOL g_bEndServer = FALSE;	// set to TRUE on CTRL-C

global char* g_Port = DEFAULT_PORT;
global SOCKET g_sdListen = INVALID_SOCKET;
global HANDLE g_hIOCP = INVALID_HANDLE_VALUE;

int myprintf(const char*lpFormat, ...);

void __cdecl main(int argc, char*argv[])
{
	SYSTEM_INFO SystemInfo;
	WSADATA WSAData;
	SOCKET sdAccept = INVALID_SOCKET;
	LPPER_SOCKET_CONTEXT lpPerSocketContext = NULL;
	DWORD dwRecvNumBytes = 0;
	DWORD dwFlags = 0;
	INT nRet = 0;

	for (int i = MAX_WORKER_THREAD; i--;)
		g_ThreadHandles[i] = INVALID_HANDLE_VALUE;

	if (!ValidOptions(argc, argv))
		return;

	if (!SetConsoleCtrlHandler(CtrlHandler, TRUE))
	{
		myprintf("SetConsoleCtrlHandler() failed to install console handler: %d\n", GetLastError());
		return;
	}

	GetSystemInfo(&SystemInfo);
	g_dwThreadCount = SystemInfo.dwNumberOfProcessors * 2;
	if ((nRet = WSAStartup(MAKEWORD(2, 2), &WSAData)) != 0)
	{
		myprintf("WSAStartup() failed: %d\n", nRet);
		SetConsoleCtrlHandler(CtrlHandler, FALSE);
		return;
	}
	printf("SERVER: WSAStartup Succedded.\n");

	__try
	{
		InitializeCriticalSection(&g_CriticalSection);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		myprintf("InitializeCriticalSection raised exception.\n");
		return;
	}

	while (g_bRestart)
	{
		g_bRestart = FALSE;
		g_bEndServer = FALSE;

		__try
		{
			// Create IOCP handle
			g_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
			if (g_hIOCP == NULL)
			{
				myprintf("CreateIoCompletionPort() failed to create I/O completion port: %d\n",
					GetLastError());
				__leave;
			}
			printf("SERVER: CreateIoCompletionPort Succedded.\n");

			for (DWORD dwCPU = 0; dwCPU < g_dwThreadCount; dwCPU++)
			{
				/*
					Create worker threads to service the overlapped I/O requests. The decision
					to create 2 worker threads per CPU in the system is a heuristic. Also,
					note that thread handles are closed right away, because we will not need
					them and the worker threads will continue to exceute,
				*/
				HANDLE hThread = INVALID_HANDLE_VALUE;
				DWORD dwThreadId = 0;
				hThread = CreateThread(NULL, 0, WorkerThread, g_hIOCP, 0, &dwThreadId);
				if (hThread == NULL)
				{
					myprintf("CreateThread() failed to create worker thread: %d\n", GetLastError());
					__leave;
				}

				printf("SERVER: CreateThread Succedded.\n");
				g_ThreadHandles[dwCPU] = hThread;
				hThread = INVALID_HANDLE_VALUE;
			}
			if (!CreateListenSocket())
				__leave;
			
			while (TRUE)
			{
				// Loop forever accepting connections from clients until console shuts down.
				SOCKADDR_IN sockAddr;
				
				sdAccept = WSAAccept(g_sdListen, (SOCKADDR*)&sockAddr, NULL, NULL, 0);
				
				if (sdAccept == SOCKET_ERROR)
				{
					/*	If user hits CTRL+C or CTRL+Brk or console window is closed,
						the control handler will close the g_sdListen socket. The above
						WSAAccept call will fail and we thus break out the loop
					*/
					myprintf("WSAAccept() failed: %d\n", WSAGetLastError());
					__leave;
				}
				getsockname(sdAccept, (SOCKADDR *)&sockAddr, (int *)sizeof(sockAddr));
				char *ip = inet_ntoa(sockAddr.sin_addr);
				printf("SERVER: Accepted Connection from :  %s\n", ip);

				/*
					We add the just returned socket descriptor to the IOCP along with its
					associated key data. Also the global list of context structures
					(the key data) gets added to a global list.
				*/

				lpPerSocketContext = UpdateCompletionPort(sdAccept, ClientIoRead, TRUE);
				if (lpPerSocketContext == NULL)
					__leave;

				/*
					if a CTRL-C was pressed "after" WSAAccept returns, the CTRL-C handler
					wil have set this flag and we can break out of the loop here before
					we go ahead and post another read (but after we have added it to the
					list of sockets to close).
				*/
				if (g_bEndServer)
					break;

				// Post initial receive on this socket THIS IS WHERE WE GET THE INITIAL DATAA
				nRet = WSARecv(
					sdAccept,
					&(lpPerSocketContext->pIOContext->Wsabuf),
					1,
					&dwRecvNumBytes,
					&dwFlags,
					&(lpPerSocketContext->pIOContext->Overlapped),
					NULL);
				if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError()))
				{
					myprintf("WSARecv() Failed: %d\n", WSAGetLastError());
					CloseClient(lpPerSocketContext, FALSE);
				}
				/*else if (g_bVerbose)
					printf("This is the buffer for the inital recv: \n%s", lpPerSocketContext->pIOContext->Wsabuf.buf);*/
			} // while
			
		} // try
		__finally
		{
			g_bEndServer = TRUE;
			
			// Cause worker threads to exit
			if (g_hIOCP)
			{
				for (DWORD i = 0; i < g_dwThreadCount; i++)
				{
					PostQueuedCompletionStatus(g_hIOCP, 0, 0, NULL);
				}
			}

			//Make sure worker threads exits.

			if (WAIT_OBJECT_0 != WaitForMultipleObjects(g_dwThreadCount, g_ThreadHandles, TRUE, 1000))
			{
				myprintf("WaitForMultipleObjects() failed: %d\n", GetLastError());
			}
			else
			{
				for (DWORD i = 0; i < g_dwThreadCount; i++)
				{
					if (g_ThreadHandles[i] != INVALID_HANDLE_VALUE)
						CloseHandle(g_ThreadHandles[i]);
					g_ThreadHandles[i] = INVALID_HANDLE_VALUE;
				}
			}
			
			CtxtListFree();

			if (g_hIOCP)
			{
				CloseHandle(g_hIOCP);
				g_hIOCP = NULL;
			}

			if (g_sdListen != INVALID_SOCKET)
			{
				closesocket(g_sdListen);
				g_sdListen = INVALID_SOCKET;
			}

			if (sdAccept != INVALID_SOCKET)
			{
				closesocket(sdAccept);
				sdAccept = INVALID_SOCKET;
			}

		}// finally
		
		if (g_bRestart)
		{
			myprintf("\niocpserver is restarting...\n");
		}
		else
		{
			myprintf("\niocpserver is exiting...\n");
		}

	}// while(g_bRestart)
	
	DeleteCriticalSection(&g_CriticalSection);
	WSACleanup();
	SetConsoleCtrlHandler(CtrlHandler, FALSE);


}

BOOL 
ValidOptions(int argc, char * argv[])
{
	BOOL bRet = TRUE;
	for (int i = 1; i < argc; i++)
	{
		if ((argv[i][0] == '-') || (argv[i][0] == '/'))
		{
			switch (tolower(argv[i][1]))
			{
			case 'e':
			{ 
				// Format is -e:port, the port number starts at char index 3
				if (strlen(argv[i]) > 3)
				{
					g_Port = &argv[i][3];
				}
			}break;
			case 'v':
			{
				g_bVerbose = TRUE;
			}
			break;
			case '?':
			{
				myprintf("Usage:\n iocpserver [-p:port] [-v] [-?]\n");
				myprintf("	-e:port\tSpecify echoing port number\n");
				myprintf("	-v\t\tVerbose\n");
				myprintf("	-?\t\tDisplay this help\n");
				bRet = FALSE;

			}
			break;
			default:
			{
				myprintf("Unknown options flag %s\n", argv[i]);
				bRet = FALSE;
			}
			break;
			}
		
		}
	}
	return(bRet);
}

/*
	Our own printf. This is done because calling printf from multiple
	threads can AV. The standard out for WriteConsole is buffered...
*/
int 
myprintf(const char * lpFormat, ...)
{

	int nLen = 0;
	int nRet = 0;
	char cBuffer[512];
	va_list arglist;
	HANDLE hOut = NULL;
	HRESULT hRet;

	ZeroMemory(cBuffer, sizeof(cBuffer));
	
	va_start(arglist, lpFormat);

	nLen = lstrlenA(lpFormat);
	hRet = StringCchVPrintfA(cBuffer, 512, lpFormat, arglist);

	if (nRet >= nLen || GetLastError() == 0)
	{
		hOut = GetStdHandle(STD_OUTPUT_HANDLE);
		if (hOut != INVALID_HANDLE_VALUE)
		{
			WriteConsoleA(hOut, cBuffer, lstrlenA(cBuffer),(LPDWORD)&nLen, NULL);
		}
	
	}

	return nLen;
}

BOOL WINAPI 
CtrlHandler(DWORD dwEvent)
{
	
	switch (dwEvent)
	{
	case CTRL_BREAK_EVENT:
		g_bRestart = TRUE;
	case CTRL_C_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
	case CTRL_CLOSE_EVENT:
	{
		if (g_bVerbose)
			myprintf("CtrlHandler: closing listening socket\n");

		// cause the accept in the main thread loop to fail

		/* 
			We want to make closesocket the last call in the handler
			because it will cause the WSAAccept to return in the main 
			thread
			
		*/
		SOCKET sockTemp = g_sdListen;
		g_sdListen = INVALID_SOCKET;
		g_bEndServer = TRUE;
		closesocket(sockTemp);
		sockTemp = INVALID_SOCKET;


	}
	break;
	default:
		// Unknown type -- better pass it on.
		return (FALSE);

	}
	return (TRUE);
}

// Create a listening socket.
BOOL CreateListenSocket(void)
{
	int nRet = 0;
	int nZero = 0;
	ADDRINFO hints = {};
	LPADDRINFO addrlocal = NULL;

	// Resolve the interface
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_IP;

	if (getaddrinfo(NULL, g_Port, &hints, &addrlocal) != 0)
	{
		myprintf("getaddrinfo() failed with error %d\n", WSAGetLastError());
		return(FALSE);
	}

	

	g_sdListen = WSASocket(addrlocal->ai_family, 
						addrlocal->ai_socktype, 
						addrlocal->ai_protocol, 
						NULL, 
						0, 
						WSA_FLAG_OVERLAPPED);

	if (g_sdListen == INVALID_SOCKET)
	{
		myprintf("WSASocket(g_sdListen) failed: %d\n", WSAGetLastError());
		return(FALSE);
	}
	nRet = bind(g_sdListen, addrlocal->ai_addr, (int)addrlocal->ai_addrlen);
	if (nRet == SOCKET_ERROR)
	{
		myprintf("bind() failed: %d\n", WSAGetLastError());
		return(FALSE);
	}
	myprintf("SERVER: bound on (%s:%s)\n", addrlocal->ai_addr, g_Port);

	nRet = listen(g_sdListen, 5);
	if (nRet == SOCKET_ERROR)
	{
		myprintf("listen() failed: %d\n", WSAGetLastError());
		return(FALSE);
	}

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
	nZero = 0;
	nRet = setsockopt(g_sdListen, SOL_SOCKET, SO_SNDBUF, (char*)&nZero, sizeof(nZero));
	if (nRet == SOCKET_ERROR)
	{
		myprintf("setsocketopt(SNDBUF) failed: %d\n", WSAGetLastError());
		return(FALSE);
	}

	/*
	Don't disable recieve buffering. This will cause poor network
	performance since if no recieve is posted and no recieve buffers,
	the TCP stack will set the window size to zero and the peer will
	no longer be allowed to send data.

	Do not set a linger value...especially don't set it to an abortive
	close. If you set abortive close and there happens to be a bit of
	data remaining to be transfered (or data that has not been
	acknowledged by the peer), the connection will be forcefully reset
	and will lead to a loss of data (i.e. the peer won't get the last
	bit of data). This is BAD. If you are worried about malicious
	clients connecting and then not sending or receiving, the server
	should maintain a timer or each connection. If after some point,
	the server deems a connetion is "stale" it can then set linger to
	be abortive and close the connection.
	*/

	/*
	LINGER lingerStruct;

	lingerStruct.l_onoff = 1;
	lingerStruct.l_linger = 0;

	nRet = setsockopt(
	g_sdListen,
	SOL_SOCKET,
	SO_LINGER,
	(char *)&lingerStruct,
	sizeof(lingerStruct) );
	if( nRet == SOCKET_ERROR )
	{
	myprintf("setsockopt(SO_LINGER) failed: %d\n", WSAGetLastError());
	return(FALSE);
	}


	*/
	freeaddrinfo(addrlocal);

	return(TRUE);
}

/*
	Worker thread that handles all I/O requests on any socket handle added to the IOCP.
*/
DWORD WINAPI 
WorkerThread(LPVOID WorkThreadContext)
{
	HANDLE hIOCP = (HANDLE)WorkThreadContext;
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
		printf("SERVER: THREAD: GetQueuedCompletionStatus() Succedded.\n");
		if (!bSuccess)
			myprintf("GetQueuedCompletionStatus() failed: %d\n", GetLastError());
	

		if (lpPerSocketContext == NULL)
		{
			//CTRL-C handler used PostQueuedCompletitionStatus to post an I/O packet with
			// a NULL CompletionKey (or if we get on for any reason). It is time to exit.
			return(0);
		}

		if (g_bEndServer)
		{
			// Main thread will do alll cleanup needed - see finally block
			return(0);
		}

		if (!bSuccess || (bSuccess && (dwIoSize == 0)))
		{
			// Client connection dropped, continue to service remaining ( and possibly
			// new) client connections
			CloseClient(lpPerSocketContext, FALSE);
			continue;
		}

		/*
			Determine what type of IO packet has completed by checking PER_IO_CONTEXT
			associated with this socket. This will detemine what action to take.
		*/
		lpIOContext = (LPPER_IO_CONTEXT)lpOverlapped;

		switch (lpIOContext->IOOperation)
		{
			case ClientIoRead:
			{

				
				/*
					a read operation has completed, post a write operation to echo the
					data back to the client using the same data buffer.
				*/
		/*		if (lpPerSocketContext->pCtxtForward)
					lpPerSocketContext->pIOContext->Wsabuf.buf = lpPerSocketContext->pCtxtForward->pIOContext->Buffer;
				else if(lpPerSocketContext->pCtxtBack)
					lpPerSocketContext->pIOContext->Wsabuf.buf = lpPerSocketContext->pCtxtBack->pIOContext->Buffer;*/

				LPPER_IO_CONTEXT lpIOContext = lpPerSocketContext->pIOContext;
				lpIOContext->IOOperation = ClientIoWrite;
				lpIOContext->nTotalBytes = dwIoSize;
				lpIOContext->nSentBytes = 0;
				lpIOContext->Wsabuf.len = dwIoSize;
				dwFlags = 0;

				if (lpIOContext->Wsabuf.buf[0] == '-')
				{
					sprintf(lpIOContext->Wsabuf.buf, "Options!");
				}

				INT nRet = WSASend(lpPerSocketContext->Socket,
					&lpIOContext->Wsabuf,
					1,
					&dwSendNumBytes,
					dwFlags,
					&(lpIOContext->Overlapped),
					NULL);

				if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError()))
				{
					myprintf("WSASend() failed: %d\n", WSAGetLastError());
					CloseClient(lpPerSocketContext, FALSE);
				}
				else if (g_bVerbose)
				{
					printf("SERVER: THREAD: read operation completed\nSENDING THIS: %s\n", lpIOContext->Wsabuf.buf);
					//	myprintf("This Buffer is ready to get sent back to the client\nBuffer: %s", "Hello");
					//myprintf("WorkerThread %d: Socket(%d) Recv completed (%d bytes), Send posted\n", GetCurrentThreadId(), lpPerSocketContext->Socket, dwIoSize);

				}
			}
			break;
			case ClientIoWrite:
			{
				
				/*
					A write operation has completed, determine if all the data intended to be sent 
					actually was sent.
				*/
				lpIOContext->IOOperation = ClientIoWrite;
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
						myprintf("WSASend() failed: %d\n", WSAGetLastError());
						CloseClient(lpPerSocketContext, FALSE);
					}
					else if (g_bVerbose)
					{
						printf("SERVER: THREAD: write operation completed\n");
						//printf("SERVER: THREAD: WorkerThread %d: Socket(%d) Send partially completed (%d bytes), Recv posted\n", GetCurrentThreadId(), lpPerSocketContext->Socket,dwIoSize);

					}
				}
				else
				{
					// Previous write operation completed for this socket, post another recv
					lpIOContext->IOOperation = ClientIoRead;

					dwRecvNumBytes = 0;
					dwFlags = 0;
					buffRecv.buf = lpIOContext->Buffer;
					buffRecv.len = MAX_BUF_SIZE;

					nRet = WSARecv(
						lpPerSocketContext->Socket,
						&buffRecv,
						1,
						&dwRecvNumBytes,
						&dwFlags,
						&lpIOContext->Overlapped,
						NULL);

					if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError()))
					{
						myprintf("WSARecv() failed: %d\n", WSAGetLastError());
						CloseClient(lpPerSocketContext, FALSE);
					}
					else if (g_bVerbose)
					{
						printf("SERVER: THREAD: recv operation completed\n");
					/*	printf("WorkerThread %d: Socket(%d) Send completed (%d bytes), Recv posted\nBuffer:%s\n",
							GetCurrentThreadId(), lpPerSocketContext->Socket, dwIoSize, buffRecv.buf);*/
					}
				}
			}
			break;
		}

		

	}
	return(0);
}


/*
	Allocate a context structures for the socket and add the socket to the IOCP.
	Additionally, add the context structure to the global list of context structures.
*/
LPPER_SOCKET_CONTEXT UpdateCompletionPort(SOCKET s, IO_OPERATION ClientIo, BOOL bAddToList)
{
	LPPER_SOCKET_CONTEXT lpPerSocketContext;
	lpPerSocketContext = CtxtAllocate(s, ClientIo);
	if (lpPerSocketContext == NULL)
		return(NULL);
	g_hIOCP = CreateIoCompletionPort((HANDLE)s, g_hIOCP, (DWORD_PTR)lpPerSocketContext, 0);
	if (g_hIOCP == NULL)
	{
		myprintf("CreateIoCompletionPort() failed: %d\n", GetLastError());
		if (lpPerSocketContext->pIOContext)
			xfree(lpPerSocketContext->pIOContext);
		xfree(lpPerSocketContext);
		return(NULL);
	}

	// The listening socket context (bAddToList is FALSE) is not added to the list
	// All other socket contexts are added to the list.
	if (bAddToList)
		CtxtListAddTo(lpPerSocketContext);

	if (g_bVerbose)
		myprintf("SERVER: UpdateCompletionPort: Socket(%d) added to the IOCP\n", lpPerSocketContext->Socket);

	return(lpPerSocketContext);
}

/*
	Close down a connection with a client. This involves closing the socket 
	(when initiated as a result of CTRL-C the socket closure is not graceful).
	Additionally, any context data associated with that socket is free'd.
*/
VOID CloseClient(LPPER_SOCKET_CONTEXT lpPerSocketContext, BOOL bGraceful)
{
	
	__try
	{
		EnterCriticalSection(&g_CriticalSection);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		myprintf("EnterCriticalSection raised an exception.\n");
		return;
	}

	

	if (lpPerSocketContext)
	{
		if (g_bVerbose)
		{
			printf("CloseClient: Socket(%d) connection closing (graceful=%s)\n",
				lpPerSocketContext->Socket, (bGraceful ? "TRUE" : "FALSE"));
		}

		if (!bGraceful)
		{
			// Force the subsequent closesocket to be abortative.
			LINGER lingerStruct;
			lingerStruct.l_onoff = 1;
			lingerStruct.l_linger = 0;
			setsockopt(lpPerSocketContext->Socket, SOL_SOCKET, SO_LINGER,
				(char*)&lingerStruct, sizeof(lingerStruct));
		}

		closesocket(lpPerSocketContext->Socket);
		lpPerSocketContext->Socket = INVALID_SOCKET;
		CtxtListDeleteFrom(lpPerSocketContext);
		lpPerSocketContext = NULL;
	}
	else
	{
		myprintf("CloseClient: lpPerSocketContext is NULL\n");
	}

	LeaveCriticalSection(&g_CriticalSection);
}

LPPER_SOCKET_CONTEXT CtxtAllocate(SOCKET s, IO_OPERATION ClientIo)
{
	LPPER_SOCKET_CONTEXT lpPerSocketContext;
	__try
	{
		EnterCriticalSection(&g_CriticalSection);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		myprintf("EnterCriticalSection raised an exception.\n");
		return(NULL);
	}

	
	
	lpPerSocketContext = (LPPER_SOCKET_CONTEXT)xmalloc(sizeof(PER_SOCKET_CONTEXT));
	if (lpPerSocketContext)
	{
		lpPerSocketContext->pIOContext = (LPPER_IO_CONTEXT)xmalloc(sizeof(PER_IO_CONTEXT));
		if (lpPerSocketContext->pIOContext)
		{
			lpPerSocketContext->Socket = s;
			lpPerSocketContext->pCtxtBack = NULL;
			lpPerSocketContext->pCtxtForward = NULL;

			lpPerSocketContext->pIOContext->Overlapped.Internal = 0;
			lpPerSocketContext->pIOContext->Overlapped.InternalHigh= 0;
			lpPerSocketContext->pIOContext->Overlapped.Offset = 0;
			lpPerSocketContext->pIOContext->Overlapped.OffsetHigh = 0;
			lpPerSocketContext->pIOContext->Overlapped.hEvent = NULL;
			lpPerSocketContext->pIOContext->IOOperation = ClientIo;
			lpPerSocketContext->pIOContext->pIOContextForward = NULL;
			lpPerSocketContext->pIOContext->nTotalBytes = 0;
			lpPerSocketContext->pIOContext->nSentBytes= 0;
			lpPerSocketContext->pIOContext->Wsabuf.buf = lpPerSocketContext->pIOContext->Buffer;
			lpPerSocketContext->pIOContext->Wsabuf.len = sizeof(lpPerSocketContext->pIOContext->Buffer);

			ZeroMemory(lpPerSocketContext->pIOContext->Wsabuf.buf, lpPerSocketContext->pIOContext->Wsabuf.len);

		}
		else
		{
			xfree(lpPerSocketContext);
			myprintf("HeapAlloc() PER_IO_CONTEXT failed: %d\n", GetLastError());
		}
	}
	else
	{
		myprintf("HeapAlloc() PER_SOCKET_CONTEXT failed: %d\n", GetLastError());
	}
	LeaveCriticalSection(&g_CriticalSection);
	return(lpPerSocketContext);
}

// Free all context structure in the global list of context structures.

VOID CtxtListFree()
{
	LPPER_SOCKET_CONTEXT pTemp1, pTemp2;
	__try
	{
		EnterCriticalSection(&g_CriticalSection);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		myprintf("EnterCriticalSection raised an exception.\n");
		return;
	}

	pTemp1 = g_pCtxList;
	while (pTemp1)
	{
		pTemp2 = pTemp1->pCtxtBack;
		CloseClient(pTemp1, FALSE);
		pTemp1 = pTemp2;
	}
	LeaveCriticalSection(&g_CriticalSection);
	return;
}

// Add a client connection context structure to the global list of context structures.

VOID CtxtListAddTo(LPPER_SOCKET_CONTEXT lpPerSocketContext)
{
	LPPER_SOCKET_CONTEXT pTemp;
	__try
	{
		EnterCriticalSection(&g_CriticalSection);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		myprintf("EnterCriticalSection raised an exception.\n");
		return;
	}

	if (g_pCtxList == NULL)
	{
		// add the first node to the linked list
		lpPerSocketContext->pCtxtBack = NULL;
		lpPerSocketContext->pCtxtForward = NULL;
		g_pCtxList = lpPerSocketContext;
	}
	else
	{
		// add the node to head of list
		pTemp = g_pCtxList;
		g_pCtxList = lpPerSocketContext;
		lpPerSocketContext->pCtxtBack = pTemp;
		lpPerSocketContext->pCtxtBack = NULL;
		pTemp->pCtxtForward = lpPerSocketContext;
	}

	LeaveCriticalSection(&g_CriticalSection);
}

VOID CtxtListDeleteFrom(LPPER_SOCKET_CONTEXT lpPerSocketContext)
{
	LPPER_SOCKET_CONTEXT pBack;
	LPPER_SOCKET_CONTEXT pForward;
	LPPER_IO_CONTEXT	pNextIO = NULL;
	LPPER_IO_CONTEXT	PTempIO = NULL;

	__try
	{
		EnterCriticalSection(&g_CriticalSection);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		myprintf("EnterCriticalSection raised an exception.\n");
		return;
	}

	if (lpPerSocketContext)
	{
		pBack = lpPerSocketContext->pCtxtBack;
		pForward = lpPerSocketContext->pCtxtForward;

		if ((pBack == NULL) && (pForward == NULL))
		{
			//This is the only node in the list to delete
			g_pCtxList = NULL;
		}
		else if ((pBack == NULL) && (pForward != NULL))
		{
			// This is the start node in the list to delete
			pForward->pCtxtBack = NULL;
			g_pCtxList = pForward;
		}
		else if ((pBack != NULL) && (pForward == NULL))
		{
			// This is the end node in the list to delete
			pBack->pCtxtForward = NULL;
		}
		else if (pBack && pForward)
		{
			// Neither start node nor end node in the list
			pBack->pCtxtForward = pForward;
			pForward->pCtxtBack = pBack;
		}

		// Free all i/o context structures per socket
		PTempIO = (LPPER_IO_CONTEXT)(lpPerSocketContext->pIOContext);
		do
		{
			pNextIO = (LPPER_IO_CONTEXT)(PTempIO->pIOContextForward);
			if (pNextIO)
			{
				/*
					The overlapped structure is safe to free when only posted i/o
					has completed. Here we only need to test those posted but not yet
					received by PQCS in the shutdown process.
				*/
				if (g_bEndServer)
				{
					while (!HasOverlappedIoCompleted((LPOVERLAPPED)PTempIO)) Sleep(0);
				}
				xfree(PTempIO);
				PTempIO = NULL;
			}
			PTempIO = pNextIO;
		} while (pNextIO);
		xfree(lpPerSocketContext);
		lpPerSocketContext = NULL;
	}
	else
	{
		myprintf("CtxListDeleteFrom: lpPerSocketContext is NULL\n");
	}
	LeaveCriticalSection(&g_CriticalSection);
}


