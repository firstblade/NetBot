#pragma once
#include "afxcmn.h"

class CSortListCtrl : public CListCtrl
{
public:
	CSortListCtrl(void);
	~CSortListCtrl(void);

	enum DATA_TYPE { INT_TYPE = 0, STRING_TYPE, DOUBLE_TYPE };
	//是否为升序
	bool m_bAsc;
	//当前排列的序
	int m_nSortedCol;

	afx_msg void OnLvnColumnclick(NMHDR *pNMHDR, LRESULT *pResult);
	DECLARE_MESSAGE_MAP()
};

int CALLBACK ListCompare(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
