#ifndef _ClientForProxy
#define _ClientForProxy
#include "../StartupInfo.h"
#include "Proxy.h"
#include "TopSites.h"
#include "GuestBook.h"
#include "MakeUrl.h"
#include "index.h"
#include "AbuseError.h"

class ClientForProxy : public startup::CClient
{
	//static vector<string> m_vProxyDNS;
	//static vector<startup::CRegisteredDNS> m_vRegisteredDNS;
	CSSPProxy m_Proxy;

	unordered_map<string, shared_ptr<proxy_site::CTemplateSSP>> m_ptrPagesGET, m_ptrPagesPOST;
	
	bool StartProxy(string strName, bool *pbRet, vector<BYTE> *pInBuffer, vector<BYTE> *pOutBuffer);
	bool OnSimpleServerPluginGet(vector<BYTE> *pOutBuffer);
	bool OnSimpleServerPluginPost(vector<BYTE> *pInBuffer, vector<BYTE> *pOutBuffer);

	static void UpdateRegisteredDNS();

public:
	virtual bool ContinueGet(vector<BYTE> *pOutBuffer);
	virtual bool ContinuePost(vector<BYTE> *pInBuffer, vector<BYTE> *pOutBuffer);
	static bool NeadCGI(const string &strURL, const string &strQuery, const map<string, string> &mapNameToValue);

	ClientForProxy()
	{
#ifdef _DEBUG
		__asm nop;
#endif
	}
	ClientForProxy(int nID, vector<string> &vQueryes, string strURI, unsigned long long llLength, const string strBoundary, const string strContentType,
		const string strIP, const string strQuery, const map<string, string> &mapNameToValue, const bool bIsSSL) :
		CClient(nID, vQueryes, strURI, llLength, strBoundary, strContentType, strIP, strQuery, mapNameToValue, bIsSSL)
	{
		m_ptrPagesGET["index.ssp"] = shared_ptr<proxy_site::CTemplateSSP>(new proxy_site::CIndexSSP());
		m_ptrPagesGET["guestbook.ssp"] = shared_ptr<proxy_site::CTemplateSSP>(new proxy_site::CGuestBookSSP());
		m_ptrPagesGET["top.ssp"] = shared_ptr<proxy_site::CTemplateSSP>(new proxy_site::CTopSitesSSP());
		m_ptrPagesGET["redirect_error.ssp"] = shared_ptr<proxy_site::CTemplateSSP>(new proxy_site::CAbuseErrorSSP());
		
		m_ptrPagesPOST["make_short_url.ssp"] = shared_ptr<proxy_site::CTemplateSSP>(new proxy_site::CMakeUrlSSP());
	}
};

#endif