//
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include "SocketHandle.h"

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

void
IHandle::Set(int iFd)
{
	if (iFd < 1) {
		return;
	}
	
	if (m_iFd > 0) {
		close(m_iFd);	
	}
	
	m_iFd = iFd;
}

int
IHandle::Add(int iEpoll)
{
	if (iEpoll < 1) {
		return -1;	
	}
	
	m_ee.events = EPOLLIN;
	if (epoll_ctl(iEpoll, EPOLL_CTL_ADD, m_iFd, &m_ee) == -1) {
		if (errno == EEXIST) {
			epoll_ctl(iEpoll, EPOLL_CTL_MOD, m_iFd, &m_ee);	
		} else {
			OnClose();
			return -1;
		}
	}
	
	m_iEpoll = iEpoll;
	
	return 0;
}

void
IHandle::Del()
{
	if (m_iEpoll > 0) {
		epoll_ctl(m_iEpoll, EPOLL_CTL_DEL, m_iFd, &m_ee);	
		m_iEpoll = -1;
	}
}

void
IHandle::OnClose()
{
	if (m_iFd > 0) {
		Del();
		
		close(m_iFd);
		m_iFd = -1;
	}
}

