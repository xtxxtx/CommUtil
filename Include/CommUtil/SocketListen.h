// SocketListen.h
//
#ifndef	SOCKETLISTEN_H_
#define	SOCKETLISTEN_H_

#include <stdint.h>

#include "CommUtil/Thread.h"


//////////////////////////////////////////////////////////////////////////
class CSocketListen : public IThread
{
	typedef int(*CBAccept)(void* pHandler, int iFd, const char* pszAddr, unsigned short iPort);

public:
	CSocketListen();
	virtual ~CSocketListen();

	int				Initialize(void* pHandler, CBAccept cbAccept, const char* pszAddr, uint16_t iPort);

	void			Close();

protected:
	void			Execute();

private:
	void*			m_pHandler;
	CBAccept		m_cbAccept;

	int				m_iFd;
};

#endif	// SOCKETLISTEN_H_

