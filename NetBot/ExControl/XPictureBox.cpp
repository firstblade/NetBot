// XPictureBox.cpp : implementation file
//

#include "stdafx.h"
#include "XPictureBox.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// XPictureBox

XPictureBox::XPictureBox()
{
}

XPictureBox::~XPictureBox()
{
	if (m_hBitmap.m_hObject != NULL)
		m_hBitmap.DeleteObject();
}


BEGIN_MESSAGE_MAP(XPictureBox, CWnd)
	//{{AFX_MSG_MAP(XPictureBox)
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// XPictureBox message handlers
void XPictureBox::SetBitmap(HBITMAP hBitmap)
{
	if (hBitmap == NULL)
		return;

	if (m_hBitmap.m_hObject != NULL)
		m_hBitmap.DeleteObject();
	m_hBitmap.Attach(hBitmap);

	Invalidate();
}

void XPictureBox::CleanBitmap()
{
	if (m_hBitmap.m_hObject != NULL)
	{
		m_hBitmap.DeleteObject();
		Invalidate();
	}
}

void XPictureBox::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if (nSBCode != SB_ENDSCROLL)
	{
		SCROLLINFO hStructure;
		GetScrollInfo(SB_HORZ, &hStructure);
		hStructure.fMask = SIF_ALL;
		hStructure.nPos = nPos;
		SetScrollInfo(SB_HORZ, &hStructure);
	}

	Invalidate();

	CWnd::OnHScroll(nSBCode, nPos, pScrollBar);
}

void XPictureBox::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if (nSBCode != SB_ENDSCROLL)
	{
		SCROLLINFO vStructure;
		GetScrollInfo(SB_VERT, &vStructure);
		vStructure.fMask = SIF_ALL;
		vStructure.nPos = nPos;
		SetScrollInfo(SB_VERT, &vStructure);
	}

	Invalidate();

	CWnd::OnVScroll(nSBCode, nPos, pScrollBar);
}

void XPictureBox::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	if (m_hBitmap.m_hObject != NULL)
	{
		CDC hdc;
		hdc.CreateCompatibleDC(&dc);
		CRect rc;
		GetClientRect(&rc);
		CBitmap* hOld = hdc.SelectObject(&m_hBitmap);


		BITMAP              bitmap;
		m_hBitmap.GetBitmap(&bitmap);

		SCROLLINFO hStructure, vStructure;
		GetScrollInfo(SB_HORZ, &hStructure);
		GetScrollInfo(SB_VERT, &vStructure);
		DWORD dwX = (int)bitmap.bmWidth - rc.right;
		if (dwX > (DWORD)hStructure.nPos)
			dwX = (DWORD)hStructure.nPos;
		DWORD dwY = (DWORD)bitmap.bmHeight - rc.bottom;
		if (dwY > (DWORD)vStructure.nPos)
			dwY = (DWORD)vStructure.nPos;

		dc.BitBlt(0, 0, rc.Width(), rc.Height(), &hdc, dwX, dwY, SRCCOPY);
		dc.SelectObject(hOld);

		hStructure.fMask = SIF_ALL;
		hStructure.nMin = 0;
		hStructure.nMax = bitmap.bmWidth;
		hStructure.nPage = rc.right * rc.right / hStructure.nMax;
		SetScrollInfo(SB_HORZ, &hStructure);

		vStructure.fMask = SIF_ALL;
		vStructure.nMin = 0;
		vStructure.nMax = bitmap.bmHeight;
		vStructure.nPage = rc.bottom * rc.bottom / vStructure.nMax;
		SetScrollInfo(SB_VERT, &vStructure);
	}
	else
	{
		CRect rc;
		GetClientRect(&rc);
		dc.FillSolidRect(rc, RGB(128, 200, 255));//or   other   color
		if (m_TipText.GetLength() > 0)
		{
			rc.top += 50;
			dc.DrawText(m_TipText, &rc, DT_CENTER);
		}
	}
	// Do not call CWnd::OnPaint() for painting messages
}

bool XPictureBox::SaveBmp(const TCHAR *FileName)
{
	HBITMAP hBitmap = (HBITMAP)m_hBitmap.GetSafeHandle();
	//位图中每象素所占字节数
	WORD wBitCount;
	DWORD dwPaletteSize = 0, dwWritten = 0;
	BITMAP Bitmap;
	BITMAPFILEHEADER bmfHdr;
	BITMAPINFOHEADER bi;
	HANDLE hOldPal = NULL;

	//计算位图文件每个像素所占字节数
	HDC hDC = ::CreateDC(_T("DISPLAY"), NULL, NULL, NULL);
	int iBits = ::GetDeviceCaps(hDC, BITSPIXEL)* ::GetDeviceCaps(hDC, PLANES);
	::DeleteDC(hDC);

	if (iBits <= 1)
		wBitCount = 1;
	else if (iBits <= 4)
		wBitCount = 4;
	else if (iBits <= 8)
		wBitCount = 8;
	else
		wBitCount = 24;

	::GetObject(hBitmap, sizeof(Bitmap), (LPSTR)&Bitmap);
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = Bitmap.bmWidth;
	bi.biHeight = Bitmap.bmHeight;
	bi.biPlanes = 1;
	bi.biBitCount = wBitCount;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrImportant = 0;
	bi.biClrUsed = 0;

	DWORD dwBmBitsSize = ((Bitmap.bmWidth * wBitCount + 31) / 32) * 4 * Bitmap.bmHeight;

	//为位图内容分配内存
	HANDLE hDib = ::GlobalAlloc(GHND, dwBmBitsSize + dwPaletteSize + sizeof(BITMAPINFOHEADER));
	LPBITMAPINFOHEADER lpbi = (LPBITMAPINFOHEADER)::GlobalLock(hDib);
	*lpbi = bi;

	//处理调色板
	HANDLE hPal = GetStockObject(DEFAULT_PALETTE);
	if (hPal)
	{
		hDC = ::GetDC(NULL);
		hOldPal = ::SelectPalette(hDC, (HPALETTE)hPal, FALSE);
		RealizePalette(hDC);
	}

	//获取该调色板下新的像素值
	GetDIBits(hDC, hBitmap, 0, (UINT)Bitmap.bmHeight, (LPSTR)lpbi + sizeof(BITMAPINFOHEADER) + dwPaletteSize,
		(BITMAPINFO*)lpbi, DIB_RGB_COLORS);

	//恢复调色板
	if (hOldPal)
	{
		::SelectPalette(hDC, (HPALETTE)hOldPal, TRUE);
		RealizePalette(hDC);
		::ReleaseDC(NULL, hDC);
	}

	//创建位图文件
	HANDLE fh = CreateFile(FileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

	if (fh == INVALID_HANDLE_VALUE)
		return FALSE;

	//设置位图文件头
	bmfHdr.bfType = 0x4D42;     //"BM"
	DWORD dwDIBSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwPaletteSize + dwBmBitsSize;
	bmfHdr.bfSize = dwDIBSize;
	bmfHdr.bfReserved1 = 0;
	bmfHdr.bfReserved2 = 0;
	bmfHdr.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER) + dwPaletteSize;
	//写入位图文件头
	WriteFile(fh, (LPSTR)&bmfHdr, sizeof(BITMAPFILEHEADER), &dwWritten, NULL);
	//写入位图文件其余内容
	WriteFile(fh, (LPSTR)lpbi, dwDIBSize, &dwWritten, NULL);
	//清除
	GlobalUnlock(hDib);
	GlobalFree(hDib);
	CloseHandle(fh);

	return TRUE;
}

void XPictureBox::SetTipText(CString strText)
{
	m_TipText = strText;
}