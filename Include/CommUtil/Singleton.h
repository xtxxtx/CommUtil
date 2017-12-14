// Singleton.h

#ifndef	SINGLETON_H_
#define	SINGLETON_H_


#define	SINGLETON_DECLARE(cls) \
	public:\
		static cls*		Instance();\
		static void		Release();\
	private:\
		cls(const cls&);\
		cls& operator=(const cls&);\
		static cls*		m_pThis;

#define	SINGLETON_IMPLEMENT(cls) \
	cls* cls::m_pThis = 0;\
	cls* cls::Instance() {\
		static cls* pThis = new cls();\
		if (m_pThis == 0) {\
			m_pThis = pThis;\
		}\
		return m_pThis;\
	}\
	void cls::Release() {\
		if (m_pThis) {\
			m_pThis->Close();\
			delete m_pThis;\
			m_pThis = 0;\
	}
	
#endif	// SINGLETON_H_

