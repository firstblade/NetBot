#include "stdafx.h"
#include "Command.h"

//����TCP�������
BOOL TurnonKeepAlive(SOCKET s, UINT nKeepAliveSec)
{
	if (nKeepAliveSec < 1)
		return TRUE;

	BOOL bSetKeepAlive = TRUE;
	if (setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, (const char*)&bSetKeepAlive, sizeof(BOOL)) != 0)
		return FALSE;

	DWORD dwBytes;
	struct tcp_keepalive Settings;
	Settings.onoff = 1;
	Settings.keepaliveinterval = 15000; //��Ӧ��ʱ�����������ֳ�ʱ��Windows�����·��ͼ�����ֱ��5��ȫ��ʧ�ܡ�
	Settings.keepalivetime = nKeepAliveSec * 1000; //��ʼ�״�KeepAlive̽��ǰ��TCP�ձ�ʱ��

	struct tcp_keepalive Retvals = { 0 };

	if (WSAIoctl(s, SIO_KEEPALIVE_VALS, &Settings, sizeof(Settings), &Retvals, sizeof(Retvals), &dwBytes, NULL, NULL) != 0)
	{
		return FALSE;
	}
	return TRUE;
}

//��������
BOOL SendData(SOCKET s, char *data, int len)
{
	char* pData = data;
	int iHasSend = 0;
	int iLeftSend = len;

	if (len <= 0)
		return TRUE;

	while (true)
	{
		int iRet = send(s, pData, iLeftSend, 0);
		if (iRet == 0 || iRet == SOCKET_ERROR)
		{
			DbpErr("Send Data Error : %d, iRet = %d", WSAGetLastError(), iRet);
			return FALSE;
		}

		iHasSend += iRet;
		pData += iRet;
		iLeftSend -= iRet;
		if (iHasSend >= len)
			break;
	}

	return TRUE;
}

//��������
BOOL RecvData(SOCKET s, char *data, int len)
{
	char *pData = data;
	int iHasRecv = 0;
	int iLeftRecv = len;

	if (len <= 0)
		return TRUE;

	while (true)
	{
		int iRet = recv(s, pData, iLeftRecv, 0);
		if (iRet == 0 || iRet == SOCKET_ERROR)
		{
			DbpErr("Recv Data Error : %d, iRet = %d", WSAGetLastError(), iRet);
			return FALSE;
		}

		iHasRecv += iRet;
		pData += iRet;
		iLeftRecv -= iRet;
		if (iHasRecv >= len)
			break;
	}

	return TRUE;
}

BOOL SendMsg(SOCKET s, char const *pBuf, LPMsgHead lpMsgHead)
{
	//������Ϣͷ
	if (!SendData(s, (char*)lpMsgHead, sizeof(MsgHead)))
		return FALSE;

	//�鿴���ݳ���
	if (lpMsgHead->dwSize <= 0)
		return TRUE;

	//��������
	if (!SendData(s, (char*)pBuf, lpMsgHead->dwSize))
		return FALSE;

	return TRUE;
}

BOOL RecvMsg(SOCKET s, char *pBuf, LPMsgHead lpMsgHead)
{
	//������Ϣͷ
	if (!RecvData(s, (char*)lpMsgHead, sizeof(MsgHead)))
		return FALSE;

	if (lpMsgHead->dwSize <= 0)
		return TRUE;

	if (!RecvData(s, pBuf, lpMsgHead->dwSize))
		return FALSE;

	return TRUE;
}
