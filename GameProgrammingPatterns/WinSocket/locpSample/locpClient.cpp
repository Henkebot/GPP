#include <stdlib.h>
#include <stdio.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <strsafe.h>
#include <iostream>
#include <string>
#pragma comment(lib, "Ws2_32.lib")
#define global static

#define MAX_THREADS 1

#define xmalloc(s) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,(s));
#define xfree(p) {HeapFree(GetProcessHeap(),0,(p));p=NULL;}


typedef struct _OPTIONS
{
	char szHostName[64];
	char* port;
	int nTotalThreads;
	int nBufSize;
	BOOL bVerbose;
} OPTIONS, *LPOPTIONS;

typedef struct _THREADINFO
{
	HANDLE hThread[MAX_THREADS];
	SOCKET sd[MAX_THREADS];
} THREADINFO, *LPTHREADINFO;


global OPTIONS default_options = { "localhost","27015", 1, 4096, TRUE };
global OPTIONS g_Options;

global BOOL g_bEndClient = FALSE;

global THREADINFO g_ThreadInfo;
global WSAEVENT g_hCleanupEvent[1];

global BOOL ValidOptions(char* argv[], int argc);
global VOID Usage(char* szProgramname, LPOPTIONS pOptions);

global BOOL WINAPI CtrlHandler(DWORD dwEvent);

global BOOL CreateConnectedSocket(int nThreadNum);
global DWORD WINAPI EchoThread(LPVOID lpParameter);
global BOOL SendBuffer(int nThreadNum, char* outbuf);
global BOOL RecvBuffer(int nThreadNum, char* inbuf);

global int myprintf(const char* lpFormat, ...);

int __cdecl main(int argc, char* argv[])
{
	OSVERSIONINFOA verInfo = { 0 };
	WSADATA WSAData;
	DWORD dwThreadId = 0;
	DWORD dwRet = 0;
	BOOL bInitError = FALSE;
	int nThreadNum[MAX_THREADS];
	int i = 0;
	int nRet = 0;

	verInfo.dwOSVersionInfoSize = sizeof(verInfo);
	GetVersionExA(&verInfo);
	if (verInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
	{
		myprintf("Please run %s only on NT, thank you\n", argv[0]);
		return(0);
	}

	for (i = 0; i < MAX_THREADS; i++)
	{
		g_ThreadInfo.sd[i] = INVALID_SOCKET;
		g_ThreadInfo.hThread[i] = INVALID_HANDLE_VALUE;
		nThreadNum[i] = 0;
	}

	g_hCleanupEvent[0] = WSA_INVALID_EVENT;

	if (!ValidOptions(argv, argc))
	{
		return(1);
	}

	if ((nRet = WSAStartup(MAKEWORD(2, 2), &WSAData)) != 0)
	{
		myprintf("WSAStartup() failed: %d", nRet);
		return(1);
	}

	if (WSA_INVALID_EVENT == (g_hCleanupEvent[0] = WSACreateEvent()))
	{
		myprintf("WSACreateEvent() failed: %d\n", WSAGetLastError());
		WSACleanup();
		return(1);
	}


	// be able to gracefully handle CTRL-C and close handles
	if (!SetConsoleCtrlHandler(CtrlHandler, TRUE))
	{
		myprintf("SetConsoleCtrlHandler() failed: %d\n", GetLastError());
		if (g_hCleanupEvent[0] != WSA_INVALID_EVENT)
		{
			WSACloseEvent(g_hCleanupEvent[0]);
			g_hCleanupEvent[0] = WSA_INVALID_EVENT;
		}

		WSACleanup();
		return(1);
	}

	// Spawn the threads

	for (i = 0; i < g_Options.nTotalThreads && !bInitError; i++)
	{
		/*
			If CTRL-C is pressed before all the sockets have connected, 
			closure of the program could take a little while, especially
			if the server is down and we have to wait for connect to fail.
			Checking for this global flag allows us to shortcircuit that.
		*/

		if (g_bEndClient)
			break;
		else if (CreateConnectedSocket(i))
		{
			nThreadNum[i] = i;
			g_ThreadInfo.hThread[i] = CreateThread(NULL, 0, EchoThread, (LPVOID)&nThreadNum[i], 0, &dwThreadId);
			if (g_ThreadInfo.hThread[i] == NULL)
			{
				myprintf("CreateThread(%d) failed: %d\n", i, GetLastError());
				bInitError = TRUE;
				break;
			}
		}
	
	}

	if (!bInitError)
	{
		// Wait for the threads to exit

		dwRet = WaitForMultipleObjects(g_Options.nTotalThreads, g_ThreadInfo.hThread, TRUE, INFINITE);
		if (dwRet == WAIT_FAILED)
		{
			myprintf("WaitForMultipleObjects(): %d\n", GetLastError());
		}

	
	}

	myprintf("Shutdown");

	if (WSAWaitForMultipleEvents(1, g_hCleanupEvent, TRUE, WSA_INFINITE, FALSE) == WSA_WAIT_FAILED)
	{
		myprintf("WSAWaitForMultipleEvents() failed: %d\n", WSAGetLastError());
	};

	if (g_hCleanupEvent[0] != WSA_INVALID_EVENT)
	{
		WSACloseEvent(g_hCleanupEvent[0]);
		g_hCleanupEvent[0] = WSA_INVALID_EVENT;
	}
	WSACleanup();

	if (!GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0))
	{
		myprintf("GenerateConsoleCtrlEvent() failed: %d\n", GetLastError());
	};

	


	SetConsoleCtrlHandler(CtrlHandler, FALSE);
	SetConsoleCtrlHandler(NULL, FALSE);

	return(0);


}

global BOOL WINAPI CtrlHandler(DWORD dwEvent)
{
	int i = 0;
	DWORD dwRet = 0;
	
	switch (dwEvent)
	{
		case CTRL_C_EVENT:
		case CTRL_BREAK_EVENT:
		case CTRL_LOGOFF_EVENT:
		case CTRL_SHUTDOWN_EVENT:
		case CTRL_CLOSE_EVENT:
		{
			myprintf("Closing handles and sockets\n");

			// Temporarily disables processing of CTRL_C_EVENT signal.

			SetConsoleCtrlHandler(NULL, TRUE);

			g_bEndClient = TRUE;

			for (i = 0; i < g_Options.nTotalThreads; i++)
			{
				if (g_ThreadInfo.sd[i] != INVALID_SOCKET)
				{
					// Force the subsequent closesocket to be abortative.

					LINGER lingerStruct;

					lingerStruct.l_onoff = 1;
					lingerStruct.l_linger = 0;
					setsockopt(g_ThreadInfo.sd[i], 
								SOL_SOCKET, 
								SO_LINGER,
								(char*)&lingerStruct, 
								sizeof(lingerStruct));
					closesocket(g_ThreadInfo.sd[i]);
					g_ThreadInfo.sd[i] = INVALID_SOCKET;

					if (g_ThreadInfo.hThread[i] != INVALID_HANDLE_VALUE)
					{
						dwRet = WaitForSingleObject(g_ThreadInfo.hThread[i], INFINITE);
						if (dwRet == WAIT_FAILED)
						{
							myprintf("WaitForSingleObject(): %d\n", GetLastError());
						}
						CloseHandle(g_ThreadInfo.hThread[i]);
						g_ThreadInfo.hThread[i] = INVALID_HANDLE_VALUE;
					}
				}
			}
		}
		break;
		default:
			// Unknown type -- better pass it on.
			return(FALSE);
	} // switch

	WSASetEvent(g_hCleanupEvent[0]);

	return(TRUE);
}

/*
	Abstract:
	Create a socket and connect to the server process.
*/
global BOOL CreateConnectedSocket(int nThreadNum)
{
	BOOL bRet = TRUE;
	int nRet = 0;
	ADDRINFO hints = {0};
	LPADDRINFO addr_srv = NULL;

	// Resolve the interface
	hints.ai_flags = 0;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	if (getaddrinfo(g_Options.szHostName, g_Options.port, &hints, &addr_srv) != 0)
	{
		myprintf("getaddrinfo() failed with error %d\n", WSAGetLastError());
		bRet = FALSE;
	}

	if (addr_srv == NULL)
	{
		myprintf("getaddrinfo() failed to resolve/convert the interface\n");
		bRet = FALSE;
	}
	else
	{
		g_ThreadInfo.sd[nThreadNum] = socket(addr_srv->ai_family,
			addr_srv->ai_socktype,
			addr_srv->ai_protocol);
		if (g_ThreadInfo.sd[nThreadNum] == INVALID_SOCKET)
		{
			myprintf("socket() failed: %d\n", WSAGetLastError());
			bRet = FALSE;
		}
	}

	if (bRet != FALSE)
	{
		nRet = connect(g_ThreadInfo.sd[nThreadNum],
			addr_srv->ai_addr,
			(int)addr_srv->ai_addrlen);
		if (nRet == SOCKET_ERROR)
		{
			myprintf("connect(thread %d) failed: %d\n", nThreadNum, WSAGetLastError());
			bRet = FALSE;
		}
		else
		{
			myprintf("connected(thread %d)\n", nThreadNum);
		}

		freeaddrinfo(addr_srv);
	}

	return(bRet);
}

/*
	Abstract:
	This is the thread that continually sends and receives a specific
	size buffer to the server. Upon receipt of the echo from the server,
	a simple check is performed to check the integrity of the transfer.
*/

global DWORD WINAPI EchoThread(LPVOID lpParameter)
{
	char* inbuf = NULL;
	char* outbuf = NULL;
	int* pArg = (int*)lpParameter;
	int nThreadNum = *pArg;

	myprintf("Starting thread %d\n", nThreadNum);

	inbuf = (char*)xmalloc(g_Options.nBufSize);
	outbuf = (char*)xmalloc(g_Options.nBufSize);

	sprintf(outbuf, "Henrik Was Here!");
	std::string buffer;
	
	if ((inbuf) && (outbuf))
	{
		// NOTE(Henrik): data possible data loss with INT conversion to BYTE
		//FillMemory(outbuf, g_Options.nBufSize, (BYTE)(nThreadNum + 1));
		
		while (TRUE)
		{
			// Just continually send and wait for the server to echo the data
			// back. Just a simple minded comparison.
			myprintf("Please enter the message:\n");
			std::getline(std::cin, buffer);
			sprintf(outbuf, buffer.c_str());


			if (SendBuffer(nThreadNum, outbuf) && RecvBuffer(nThreadNum, inbuf))
			{
				myprintf("Response: %s\n", inbuf);
				/*if ((inbuf[0] == outbuf[0]) && inbuf[g_Options.nBufSize - 1] == outbuf[g_Options.nBufSize - 1])
				{
					if (g_Options.bVerbose)
					{
						myprintf("inBuf:\n%s", inbuf);
					}
				}
				else
				{
					myprintf("nak(%d) in[0]=%d, out[0]=%d in[%d]=%d out[%d]=%d\n",
						nThreadNum, inbuf[0], outbuf[0], g_Options.nBufSize - 1, inbuf[g_Options.nBufSize - 1],
						g_Options.nBufSize - 1, outbuf[g_Options.nBufSize - 1]);
					break;
				}*/
			}
			else
				break;

		
		} //While(TRUE)
	}

	if (inbuf)
		xfree(inbuf);
	if (outbuf)
		xfree(outbuf);

	return(TRUE);
}
/*
	Abstract:
	Send a buffer - keep sending untill the request amount of
	data has been sent or the socket has been closed or error.
*/
global BOOL SendBuffer(int nThreadNum, char * outbuf)
{
	BOOL bRet = TRUE;
	char* pBuf = outbuf;
	int nTotalSend = 0;
	int nSend = 0;

	while (nTotalSend < g_Options.nBufSize)
	{
		nSend = send(g_ThreadInfo.sd[nThreadNum], pBuf, g_Options.nBufSize - nTotalSend, 0);
		if (nSend == SOCKET_ERROR)
		{
			myprintf("send(thread=%d) failed: %d\n", nThreadNum, WSAGetLastError());
			bRet = FALSE;
			break;
		}
		else if (nSend == 0)
		{
			myprintf("connection closed\n");
			bRet = FALSE;
			break;
		}
		else
		{
			nTotalSend += nSend;
			pBuf += nSend;
		}
	}

	return(bRet);

}

/*
	Abstract:
	Receive a buffer - keep recving untill the requested amount
	of data has been received or the socket has been closed or error.
*/
global BOOL RecvBuffer(int nThreadNum, char * inbuf)
{
	BOOL bRet = TRUE;
	char* pBuf = inbuf;
	int nTotalRecv = 0;
	int nRecv = 0;

	while (nTotalRecv < g_Options.nBufSize)
	{
		nRecv = recv(g_ThreadInfo.sd[nThreadNum], pBuf, g_Options.nBufSize - nTotalRecv, 0);
		if (nRecv == SOCKET_ERROR)
		{
			myprintf("recv(thread=%d) failed: %d\n", nThreadNum, WSAGetLastError());
			bRet = FALSE;
			break;
		}
		else if (nRecv == 0)
		{
			myprintf("connection closed\n");
			bRet = FALSE;
			break;
		}
		else
		{
			nTotalRecv += nRecv;
			pBuf += nRecv;
		}
	}
	return(bRet);
}

static int myprintf(const char* lpFormat, ...)
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
			WriteConsoleA(hOut, cBuffer, lstrlenA(cBuffer), (LPDWORD)&nLen, NULL);
		}
	}
	return nLen;
}


// Abstract: 
// Verify options passed in and set options structure accordingly
global BOOL ValidOptions(char * argv[], int argc)
{
	g_Options = default_options;
	HRESULT hRet;

	for (int i = 1; i < argc; i++)
	{
		if ((argv[i][0] == '-') || (argv[i][0] == '/'))
		{
			switch (tolower(argv[i][1]))
			{
				case 'b':
				{
					if (lstrlenA(argv[i]) > 3)
					{
						g_Options.nBufSize = 1024 * atoi(&argv[i][3]);
					}

				}
				break;
				case 'e':
				{
					if (lstrlenA(argv[i]) > 3)
					{
						g_Options.port = &argv[i][3];
					}
				}
				break;
				case 'n':
				{
					if (lstrlenA(argv[i]) > 3)
					{
						hRet = StringCbCopyNA(g_Options.szHostName, 64, &argv[i][3], 64);
					}

				}
				break;

				case 't':
				{
					if (lstrlenA(argv[i]) > 3)
					{
						g_Options.nTotalThreads = min(MAX_THREADS, atoi(&argv[i][3]));
					}
				}break;

				case 'v':
				{
					g_Options.bVerbose = TRUE;
				}break;

				case '?':
				{
					Usage(argv[0], &g_Options);
					return (FALSE);
				}break;

				default:
				{
					myprintf("	unknown options flag %s\n", argv[i]);
					Usage(argv[0], &g_Options);
					return(FALSE);
				}
				break;
			} // switch (tolower(argv[i][1]))
		} //if ((argv[i][0] == '-') || (argv[i][0] == '/'))
		else
		{
			myprintf("	unknown option %s\n", argv[i]);
			Usage(argv[0], &default_options);
			return(FALSE);
		}
	}// for (int i = 1; i < argc; i++)

	return(TRUE);
}

global VOID Usage(char * szProgramname, LPOPTIONS pOptions)
{
	myprintf("usage:\n%s [-b:#] [-e:#] [-n:host] [-t:#] [-v]\n", szProgramname);
	myprintf("%s -?\n", szProgramname);
	myprintf("	-?\t\tDisplay this help\n");
	myprintf("	-b:bufsize\tSize of send/recv buffer; in 1k increments (Def:%d)\n",
		pOptions->nBufSize);
	myprintf("	-e:port\tEndpoint number (port) to use (Def:%s)\n", pOptions->port);
	myprintf("	-n:host\tAct as the client and connect to 'host' (Def:%s)\n", pOptions->szHostName);
	myprintf("	-t:#\tNumber of threads to use\n");
	myprintf("	-v\t\tVerbose, print an ack when echo received and verified\n");
}
