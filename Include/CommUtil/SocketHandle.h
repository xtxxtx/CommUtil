//
#ifndef SOCKETHANDLE_H_
#define SOCKETHANDLE_H_

#include <sys/epoll.h>

class IHandle
{
public:
	IHandle();
	virtual ~IHandle();
  
	void    Set(int iFd);
  
	int     Add(int iEpoll);
	void    Del();
  
	int&    GetFD() { return m_iFd; }
	struct epoll_event* GetEE() { return &m_ee; }
  
	void    OnClose();
  
public:
	virtual int   OnRecv()  { return -1; }
	virtual int   OnSend()  { return -1; }
  
protected:
	struct epoll_event  m_ee;
	int       m_iFd;
	int       m_iEpoll;
};

#endif  // SOCKETHANDLE_H_

