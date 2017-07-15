//////////////////////////////////////////////////////////////////////////
//IniFile.cpp
//ini�ļ���д��
//������Ȩ����
//2005��7��6��
//��������ʹ�á��޸ĺʹ��������뱣�����߰�Ȩ��Ϣ��
//////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "IniFile.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

#define         MAX_SECTION                260        //Section��󳤶�
#define         MAX_KEY                         260        //KeyValues��󳤶�
#define         MAX_ALLSECTIONS     65535    //����Section����󳤶�
#define         MAX_ALLKEYS              65535    //����KeyValue����󳤶�

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CIniFile::CIniFile()
{
}

CIniFile::~CIniFile()
{
}
//////////////////////////////////////////////////////////////////////////
//   Public Functions
//////////////////////////////////////////////////////////////////////////
BOOL CIniFile::SetPath(CString strPath)
{
	m_strPath = strPath;
	DWORD  dwFlag = GetFileAttributes((LPCTSTR)m_strPath);//����ļ��Ƿ����
	if (0xFFFFFFFF == dwFlag)//�ļ�����·�������ڣ�����FALSE
		return FALSE;
	if (FILE_ATTRIBUTE_DIRECTORY&dwFlag)//·����Ŀ¼������FALSE
		return FALSE;
	//		 AfxMessageBox(m_strPath);
	return TRUE;
}

BOOL CIniFile::Createini()//����m_strPath�е�ini�ļ�
{
	if (!m_strPath.IsEmpty())
	{
		FILE *fp = fopen(CT2A(m_strPath), "a+");
		if(fp) fclose(fp);
	}
	else
		AfxMessageBox(_T("������SetPath��������·��!"));
	return SetPath(m_strPath);
}

BOOL CIniFile::SectionExist(CString strSection)
{
	TCHAR chSection[MAX_SECTION];

	DWORD dwRetValue = GetPrivateProfileString(
		(LPCTSTR)strSection,
		NULL,
		_T(""),
		chSection,
		sizeof(chSection) / sizeof(TCHAR),
		(LPCTSTR)m_strPath);
	return (dwRetValue > 0);
}

CString CIniFile::GetKeyValue(CString strSection, CString strKey)
{
	TCHAR chKey[MAX_KEY];
	GetPrivateProfileString(
		(LPCTSTR)strSection,
		(LPCTSTR)strKey,
		_T(""),
		chKey,
		sizeof(chKey) / sizeof(TCHAR),
		(LPCTSTR)m_strPath);

	CString strKeyValue = chKey;
	return strKeyValue;
}

void CIniFile::SetKeyValue(CString strSection, CString strKey, CString strKeyValue)
{
	WritePrivateProfileString(
		(LPCTSTR)strSection,
		(LPCTSTR)strKey,
		(LPCTSTR)strKeyValue,
		(LPCTSTR)m_strPath);
}

void CIniFile::DeleteKey(CString strSection, CString strKey)
{
	WritePrivateProfileString((LPCTSTR)strSection, (LPCTSTR)strKey, NULL, (LPCTSTR)m_strPath);
}

void CIniFile::DeleteSection(CString strSection)
{
	WritePrivateProfileString(
		(LPCTSTR)strSection,
		NULL,
		NULL,          //       ���ﶼдNULL,��ɾ��Section
		(LPCTSTR)m_strPath);
}

int CIniFile::GetAllSections(CStringArray& strArrSection)
{
	int i, iPos = 0;
	TCHAR chAllSections[MAX_ALLSECTIONS];
	TCHAR chTempSection[MAX_SECTION];

	ZeroMemory(chAllSections, MAX_ALLSECTIONS * sizeof(TCHAR));
	ZeroMemory(chTempSection, MAX_SECTION * sizeof(TCHAR));

	GetPrivateProfileSectionNames(chAllSections, MAX_ALLSECTIONS, m_strPath);

	//��ΪSection�������еĴ����ʽΪ��Section1����0����Section2����0��0��
	//���������⵽��������0����break
	for (i = 0; i < MAX_ALLSECTIONS; i++)
	{
		if (chAllSections[i] == NULL && chAllSections[i + 1] == NULL)
		{
			break;
		}
	}
	i++; //         ��֤���ݶ���
	strArrSection.RemoveAll(); //         �������
	for (int j = 0; j < i; j++)
	{
		chTempSection[iPos++] = chAllSections[j];
		if (chAllSections[j] == NULL)
		{
			strArrSection.Add(chTempSection);
			ZeroMemory(chTempSection, MAX_SECTION * sizeof(TCHAR));
			iPos = 0;
		}
	}

	return strArrSection.GetSize();
}

int CIniFile::GetAllKeysAndValues(CString strSection, CStringArray&strArrKey, CStringArray&strArrKeyValue)
{
	int i, iPos = 0;
	TCHAR chAllKeysAndValues[MAX_ALLKEYS];
	TCHAR chTempkeyAndValue[MAX_KEY];

	ZeroMemory(chAllKeysAndValues, MAX_ALLKEYS * sizeof(TCHAR));
	ZeroMemory(chTempkeyAndValue, MAX_KEY * sizeof(TCHAR));

	GetPrivateProfileSection(strSection, chAllKeysAndValues, MAX_ALLKEYS, m_strPath);

	//       ��ΪSection�������еĴ����ʽΪ��Key1=KeyValue1����0����Key2=KeyValue2����0
	//       ���������⵽��������0����break
	for (i = 0; i < MAX_ALLSECTIONS; i++)
	{
		if (chAllKeysAndValues[i] == NULL)
		{
			if (chAllKeysAndValues[i] == chAllKeysAndValues[i + 1])
				break;
		}
	}

	i++;
	strArrKey.RemoveAll();
	strArrKeyValue.RemoveAll();

	for (int j = 0; j < i; j++)
	{
		chTempkeyAndValue[iPos++] = chAllKeysAndValues[j];
		if (chAllKeysAndValues[j] == NULL)
		{
			CString strTempKey = chTempkeyAndValue;
			strArrKey.Add(strTempKey.Left(strTempKey.Find('=')));
			strArrKeyValue.Add(strTempKey.Mid(strTempKey.Find('=') + 1));
			ZeroMemory(chTempkeyAndValue, MAX_KEY * sizeof(TCHAR));
			iPos = 0;
		}
	}
	return strArrKey.GetSize();
}

void CIniFile::DeleteAllSections()
{
	CStringArray strArrSection;
	int nSecNum = GetAllSections(strArrSection);
	for (int i = 0; i < nSecNum; i++)
	{
		WritePrivateProfileString(
			(LPCTSTR)strArrSection[i],
			NULL,
			NULL,
			(LPCTSTR)m_strPath);
	}
}