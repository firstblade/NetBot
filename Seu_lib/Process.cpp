#include "StdAfx.h"
#include "Process.h"
#include <tlhelp32.h>
#include "Functions.h"

void ProcessList(char *pBuf, LPMsgHead lpMsgHead)
{
	GrantPrivilege();

	typedef BOOL(WINAPI *QueryFullProcessImageName) (HANDLE hProcess, DWORD dwFlags, LPCSTR lpExeName, PDWORD lpdwSize);
	QueryFullProcessImageName LxQueryFullProcessImageName = (QueryFullProcessImageName)GetProcAddress(GetModuleHandleW(L"Kernel32.dll"), "QueryFullProcessImageNameA");
	if (LxQueryFullProcessImageName == NULL)
	{
		LxQueryFullProcessImageName = (QueryFullProcessImageName)GetProcAddress(LoadLibraryW(L"Psapi.dll"), "GetModuleFileNameExA");
	}

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == NULL)
	{
		lpMsgHead->dwCmd = CMD_PROCLISTERR;
		lpMsgHead->dwSize = 0;
		return;
	}

	PROCESSENTRY32 PInfo;
	PInfo.dwSize = sizeof(PROCESSENTRY32);

	ProcessInfo Info;
	DWORD dwLen = 0;
	DWORD bContinue = Process32First(hSnapshot, &PInfo);
	while (bContinue)
	{
		memset(&Info, 0, sizeof(ProcessInfo));
		Info.dwPid = PInfo.th32ProcessID;
		Info.dwThreads = PInfo.cntThreads;

		WideCharToMultiByte(CP_ACP, 0, PInfo.szExeFile, wcslen(PInfo.szExeFile), Info.FileName, 32, NULL, NULL);
		//lstrcpynA(Info.FileName, PInfo.szExeFile, 32);

		HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, PInfo.th32ProcessID);
		Info.dwPriClass = GetPriorityClass(hProcess);
		DWORD rt = 128;
		LxQueryFullProcessImageName(hProcess, 0, Info.FilePath, &rt);
		CloseHandle(hProcess);

		memcpy(pBuf + dwLen, &Info, sizeof(ProcessInfo));
		dwLen += sizeof(ProcessInfo);

		bContinue = Process32Next(hSnapshot, &PInfo);
	}

	CloseHandle(hSnapshot);

	lpMsgHead->dwCmd = 0;
	lpMsgHead->dwSize = dwLen;
}

void ProcessKill(char *pBuf, LPMsgHead lpMsgHead)
{
	HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, lpMsgHead->dwExtend1);
	if (hProcess != NULL)
	{
		TerminateProcess(hProcess, 0);
	}

	lpMsgHead->dwCmd = CMD_PROCKILLERR;
	lpMsgHead->dwSize = 0;

	CloseHandle(hProcess);
}
