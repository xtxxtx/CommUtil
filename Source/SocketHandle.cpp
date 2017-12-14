//
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include "CommUtil/SocketHandle.h"

IHandle::IHandle()
{
	m_iFd		= -1;
	m_iEpoll	= -1;
	
	m_ee.events	= EPOLLIN;
	m_ee.data.ptr	= this;
}

IHandle::~IHandle()
{
	if (m_iFd != -1) {
		close(m_iFd);
	}
}

