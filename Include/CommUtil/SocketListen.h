// SocketListen.h
//
#ifndef	SOCKETLISTEN_H_
#define	SOCKETLISTEN_H_

#include <stdint.h>

#include "CommUtil/Thread.h"



typedef int(*CBAccept)(void* pHandler, int iFd, const char* pszAddr, unsigned short iPort);

class CSocketListen : public IThread
{
public:
	CSocketListen(void* pHandler, CBAccept cbAccept);
	virtual ~CSocketListen();

	int				Initialize(const char* pszAddr, uint16_t iPort);

	void			Close();

protected:
	void			Execute();

private:
	void*			m_pHandler;
	CBAccept		m_cbAccept;

	int				m_iFd;
};

#endif	// SOCKETLISTEN_H_

