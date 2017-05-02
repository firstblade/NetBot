#include "stdafx.h"
#include "SystemInfo.h"

BOOL GetSystemInfo(SysInfo& info)
{
	ZeroMemory(&info, sizeof(SysInfo));

	DWORD iSize = 64;
	GetComputerNameA(info.cComputer, &iSize);

	char szSystem[32];
	OSVERSIONINFOEXA osvi;
	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEXA));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
	if (!GetVersionExA((OSVERSIONINFOA *)&osvi))
		return FALSE;

	switch (osvi.dwPlatformId)
	{
	case VER_PLATFORM_WIN32_NT:
		if (osvi.dwMajorVersion == 6)
		{
			if (osvi.dwMinorVersion == 0) lstrcpyA(szSystem, "Windows Vista");
			if (osvi.dwMinorVersion == 1) lstrcpyA(szSystem, "Windows 7");
			if (osvi.dwMinorVersion == 2) lstrcpyA(szSystem, "Windows 8");
			if (osvi.dwMinorVersion == 3) lstrcpyA(szSystem, "Windows 8.1");
		}
		else if (osvi.dwMajorVersion == 5)
		{
			if (osvi.dwMinorVersion == 0) lstrcpyA(szSystem, "Windows 2000");
			if (osvi.dwMinorVersion == 1) lstrcpyA(szSystem, "Windows XP");
			if (osvi.dwMinorVersion == 2) lstrcpyA(szSystem, "Windows 2003");
		}
		else if (osvi.dwMajorVersion == 10) lstrcpyA(szSystem, "Windows 10");
		else if (osvi.dwMajorVersion <= 4) lstrcpyA(szSystem, "Win NT");
		break;

	case VER_PLATFORM_WIN32_WINDOWS:
		if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0)
			lstrcpyA(szSystem, "Win 95");

		if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 10)
			lstrcpyA(szSystem, "Win 98");
		break;
	default:
		break;
	}

	if (IsX64System())
	{
		lstrcatA(szSystem, " x64");
	}
	else
	{
		lstrcatA(szSystem, " x86");
	}
	wsprintfA(info.cOS, "%s SP%d (Build %d)", szSystem, osvi.wServicePackMajor, osvi.dwBuildNumber);

	MEMORYSTATUSEX mem;
	mem.dwLength = sizeof(mem);

	//typedef void(WINAPI* FunctionGlobalMemoryStatusEx)(LPMEMORYSTATUS);
	//FunctionGlobalMemoryStatusEx GlobalMemoryStatusEx;
	//GlobalMemoryStatusEx = (FunctionGlobalMemoryStatusEx)GetProcAddress(GetModuleHandle("kernel32.dll"), "GlobalMemoryStatusEx");

	GlobalMemoryStatusEx(&mem);//调用函数取得系统的内存情况

	wsprintfA(info.cMemorySize, "%dMB", mem.ullTotalPhys / (1024 * 1024) + 1);

	return TRUE;
}
