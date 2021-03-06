#include "stdafx.h"
#include "Command.h"

//开启TCP保活机制
BOOL TurnonKeepAlive(SOCKET s, UINT nKeepAliveSec)
{
	if (nKeepAliveSec < 1)
		return TRUE;

	char bSetKeepAlive = TRUE;
	if (setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &bSetKeepAlive, sizeof(bSetKeepAlive)) != 0)
		return FALSE;

	DWORD dwBytes;
	struct tcp_keepalive Settings;
	Settings.onoff = 1;
	Settings.keepaliveinterval = 15000; //回应超时间隔，如果出现超时，Windows将重新发送检测包，直到5次全部失败。
	Settings.keepalivetime = nKeepAliveSec * 1000; //开始首次KeepAlive探测前的TCP空闭时间

	struct tcp_keepalive Retvals = { 0 };

	if (WSAIoctl(s, SIO_KEEPALIVE_VALS, &Settings, sizeof(Settings), &Retvals, sizeof(Retvals), &dwBytes, NULL, NULL) != 0)
	{
		return FALSE;
	}
	return TRUE;
}

//发送数据
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

//接收数据
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
	//发送消息头
	if (!SendData(s, (char*)lpMsgHead, sizeof(MsgHead)))
		return FALSE;

	//查看数据长度
	if (lpMsgHead->dwSize <= 0)
		return TRUE;

	//发送数据
	if (!SendData(s, (char*)pBuf, lpMsgHead->dwSize))
		return FALSE;

	return TRUE;
}

BOOL RecvMsg(SOCKET s, char *pBuf, LPMsgHead lpMsgHead)
{
	//接收消息头
	if (!RecvData(s, (char*)lpMsgHead, sizeof(MsgHead)))
		return FALSE;

	if (lpMsgHead->dwSize <= 0)
		return TRUE;

	if (!RecvData(s, pBuf, lpMsgHead->dwSize))
		return FALSE;

	return TRUE;
}
