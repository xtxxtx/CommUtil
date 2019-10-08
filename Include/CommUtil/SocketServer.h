// SocketServer.h

#ifndef SOCKETSERVER_H_
#define SOCKETSERVER_H_

#include <list>

#include <CommUtil/Mutex.h>
#include <CommUtil/Thread.h>


//class CSocketClient;

// typedef int(*CBConnect)(void* pHandler, CSocketClient*);
typedef void* (*CBConnect)(void* pHandler, void* pClient);
typedef int (*CBReceive)(void* pParent, const char* pszBuf, int iLen);
typedef int (*CBClose)(void* pParent, void* pClient, int iError);

//////////////////////////////////////////////////////////////////////////
class CSocketServer : public IThread
{
	class CListen : public IThread
	{
	public:
		CListen(CSocketServer* pServer);
		virtual ~CListen();

		int				Initialize(const char* pszAddr, uint16_t iPort);

		void			Close();

	protected:
		void			Execute();

	private:
		CListen();
		CListen(const CListen&);

	private:
		CSocketServer*	m_pServer;

		int				m_iFd;
	};

	class CClient
	{
		enum { BUF_SIZ=8192 };

	public:
		CClient();
		~CClient();

		friend class CSocketServer;

		int			Initialize(int iFd, const char* pszAddr, uint16_t iPort);

		void		Set(int iFd)	{ m_iFd = iFd; }
		void		Set(void* pParent)	{
			m_pParent = pParent;
		}

		int			Add(int iEpoll);
		void		Del();

		int&		GetFD() { return m_iFd; }

		int			OnRecv(void* pHander, char* pBuf, int iSize);

		void		Close();

		struct epoll_event* GetEE() { return &m_ee; }

	private:
		int			m_iFd;
		char		m_szAddr[256];
		uint16_t	m_iPort;
		int			m_iEpoll;

		struct epoll_event  m_ee;

		char*		m_pBuf;
		int32_t		m_iSize;

		void*		m_pParent;
		CBReceive	m_cbReceive;
		CBClose		m_cbClose;
	};

	typedef std::list<CClient*>	LST_CLIENT;

public:
	CSocketServer();
	~CSocketServer();

	int				Initialize(const char* pszAddr, uint16_t iPort, long lNum, void* pHandler,
						CBConnect cbConnect, CBReceive cbReceive, CBClose cbClose);


	void			Close(void* pHandler, void* pClient);

	void			Close();

	int				Send(void* pClient, const char* pszBuf, int iLen);
	
protected:
	virtual void	Execute();

	int				OnAccept(int iFd, const char* pszAddr, uint16_t iPort);

private:
	CSocketServer(const CSocketServer&);
	CSocketServer& operator=(const CSocketServer);
	
	int				OnRecv(int iFd, char* pszBuf, int iSize);

protected:
	void*			m_pHandler;
	CBConnect		m_cbConnect;
	CBReceive		m_cbReceive;
	CBClose			m_cbClose;

	CListen*		m_pListen;

	CMutex			m_mtxClientNew;
	LST_CLIENT		m_lstClientNew;

	CMutex			m_mtxClientAll;
	LST_CLIENT		m_lstClientAll;
};

#endif  // SOCKETSERVER_H_
