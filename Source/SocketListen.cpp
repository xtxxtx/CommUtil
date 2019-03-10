// SocketListen.cpp
//
#include <unistd.h>
//#include <poll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>

#include "CommUtil/Log.h"
#include "CommUtil/SocketServer.h"
#include "CommUtil/SocketListen.h"


static const int SAIN_SIZE = sizeof(struct sockaddr_in);

CSocketListen::CSocketListen(CSocketServer* pServer) : m_pServer(pServer)
{
	m_iFd	= -1;
}

CSocketListen::~CSocketListen()
{

}

int
CSocketListen::Initialize(const char* pszAddr, uint16_t iPort)
{
	if (pszAddr == NULL) {
		return -1;
	}

	if (m_iFd > 0) {
		close(m_iFd);
	}
	m_iFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_iFd < 1) {
		return -1;
	}

	do {
		int iFlag = 1;
		if (setsockopt(m_iFd, SOL_SOCKET, SO_REUSEADDR, (const char*)&iFlag, sizeof(iFlag))==-1) {
			break;
		}

		struct sockaddr_in sa;
		sa.sin_family		= AF_INET;
		sa.sin_port			= htons(iPort);
		inet_pton(AF_INET, pszAddr, &(sa.sin_addr));
		sa.sin_zero[0]		= 0;

		if (bind(m_iFd, (const sockaddr*)&sa, sizeof(sa)) == -1) {
			CLog::Instance()->Write(LOG_ERROR, "bind(%s:%d) failed. errno: %d", pszAddr, iPort, errno);
			break;
		}

		if (listen(m_iFd, SOMAXCONN) == -1) {
			break;
		}

		return Run(2);
	} while (false);

	close(m_iFd);
	m_iFd = -1;

	return -1;
}

void
CSocketListen::Close()
{
	if (m_iFd > 0) {
		close(m_iFd);
		m_iFd = -1;
	}

	WaitExit();
}

void
CSocketListen::Execute()
{
	if (m_pServer == NULL) {
		CLog::Instance()->Write(LOG_ERROR, "CSocketListen::Execute() m_pServer is NULL.");
		return;
	}

	const int BUF_SIZ = 64*1024;
	int iFd = -1;
	char szAddr[256] = {0};
	sockaddr_in sa;
	socklen_t len = SAIN_SIZE;

//	struct pollfd pfds = {m_iFd, POLLIN, 0};

	for (; m_bRun; ) {
//		if (poll(&pfds, 1, 100) < 1) {
//			continue;
//		}

		len = SAIN_SIZE;
		iFd = accept(m_iFd, (sockaddr *)&sa, &len);
		if (iFd < 1) {
			continue;
		}

		setsockopt(iFd, SOL_SOCKET, SO_RCVBUF, (const char*)&BUF_SIZ, sizeof(BUF_SIZ));
		setsockopt(iFd, SOL_SOCKET, SO_SNDBUF, (const char*)&BUF_SIZ, sizeof(BUF_SIZ));

		szAddr[0] = 0;
		inet_ntop(AF_INET, &(sa.sin_addr), szAddr, sizeof(szAddr));
		m_pServer->Create(iFd, szAddr, ntohs(sa.sin_port));
	}

	CLog::Instance()->Write(LOG_INFO, "CSocketListen::Execute() exit.");
}
