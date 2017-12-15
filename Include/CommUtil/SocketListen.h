// SocketListen.h
//
#ifndef	SOCKETLISTEN_H_
#define	SOCKETLISTEN_H_

#include <stdint.h>

#include "CommUtil/Thread.h"


class CSocketServer;

class CSocketListen : public IThread
{
public:
	CSocketListen(CSocketServer* pServer);
	virtual ~CSocketListen();

	int				Initialize(const char* pszAddr, uint16_t iPort);

	void			Close();

protected:
	void			Execute();

private:
	CSocketServer*	m_pServer;

	int				m_iFd;
};

#endif	// SOCKETLISTEN_H_

