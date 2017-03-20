#include "ClientForProxy.h"

vector<string> ClientForProxy::m_vProxyDNS;
vector<startup::CRegisteredDNS> ClientForProxy::m_vRegisteredDNS;

bool ClientForProxy::ContinueGet(vector<BYTE> *pOutBuffer)
{
	bool bRet;
	for (size_t n=0; n<m_vProxyDNS.size(); n++)
	{
		if (StartProxy(m_vProxyDNS[n], &bRet, shared_ptr<vector<BYTE> >(new vector<BYTE>).get(), pOutBuffer))
			return bRet;
	}

	string strURI = GetURI();
	if ((strURI == "/simple_server_plugin") ||
		(strURI.rfind(".ssp") == strURI.length()-4))
		return OnSimpleServerPluginGet(pOutBuffer);


	return false;
}
bool ClientForProxy::ContinuePost(vector<BYTE> *pInBuffer, vector<BYTE> *pOutBuffer)
{
	//return false;
	CClient::ContinuePost(pInBuffer, pOutBuffer);

	return OnSimpleServerPluginPost(pInBuffer, pOutBuffer);
}

void ClientForProxy::UpdateRegisteredDNS()
{
	ClientForProxy::m_vRegisteredDNS.push_back(startup::CRegisteredDNS("langtest.ru", "193.107.236.167", 8085, 8443));
	ClientForProxy::m_vRegisteredDNS.push_back(startup::CRegisteredDNS("multicoins.org", "104.236.180.129", 8088, 9443));
//#ifdef _DEBUG
//	ClientForProxy::m_vRegisteredDNS.push_back(startup::CRegisteredDNS("104.236.180.129", 8085, 8443));
//#endif
}

bool ClientForProxy::NeadCGI(const string &strURL, const string &strQuery, const map<string, string> &mapNameToValue) 
{
	if (!m_vProxyDNS.size())
	{
		//m_vProxyDNS.push_back("langtest.ru");
		m_vProxyDNS.push_back(DNS_NAME);
		//m_vProxyDNS.push_back("unblok.us");
	}
	
	string strHost = (mapNameToValue.find("Host") == mapNameToValue.end()) ? "" : mapNameToValue.at("Host");
	string strMethod = (mapNameToValue.find("Method") == mapNameToValue.end()) ? "" : mapNameToValue.at("Method");
	string strFolder = (mapNameToValue.find(strMethod) == mapNameToValue.end()) ? "" : mapNameToValue.at(strMethod);
	DEBUG_LOG("ClientForProxy::NeadCGI Host=%s; Method=%s", strHost.c_str(), strMethod.c_str());

	if (!m_vRegisteredDNS.size())
		UpdateRegisteredDNS();

	for (size_t i=0; i<m_vRegisteredDNS.size(); i++)
	{
		if (strHost.find(m_vRegisteredDNS[i].DNS()) != -1)
			return true;
	}

////////////////////////////////////////////////////////////////////////////////////////////
#ifdef _DEBUG
	if ((strFolder.find("http://") == 0) || (strFolder.find("https://") == 0))
		return true;
#else
	if ((strFolder.find("http://") == 0) || (strFolder.find("https://") == 0))
		return false; //disable open proxy
#endif
////////////////////////////////////////////////////////////////////////////////////////////

	for (size_t n=0; n<m_vProxyDNS.size(); n++)
	{
		if ((strHost.find("."+m_vProxyDNS[n]) != -1) && (strHost.find("www." +m_vProxyDNS[n]) == -1))
			return true;
	}

#ifdef _DEBUG
	if (strHost.find(" localhost") == 0)
	{
		DEBUG_LOG("My site start");

		if ((strMethod == "POST") ||
			(strQuery.length() != 0) ||
			(strURL.rfind(".ssp") == strURL.length()-4))
		{
			if ((strURL.rfind(".js") != strURL.length()-3) &&
				(strURL.rfind(".html") != strURL.length()-5))
			{
				DEBUG_LOG("My site end: return true");
				return true;
			}
		}

		DEBUG_LOG("My site end: return false");
		return false;
	}
#endif

	if (strHost.find(string(" ") + DNS_NAME) != -1 || strHost.find(string(" www.") + DNS_NAME) != -1)
	{
		DEBUG_LOG("My site start");

		if ((strMethod == "POST") ||
			(strQuery.length() != 0) ||
			(strURL.rfind(".ssp") == strURL.length()-4))
		{
			if ((strURL.rfind(".js") != strURL.length()-3) &&
				(strURL.rfind(".html") != strURL.length()-5))
			{
				DEBUG_LOG("My site end: return true");
				return true;
			}
		}

		DEBUG_LOG("My site end: return false");
		return false;
	}

	return true;

}

bool ClientForProxy::StartProxy(string strName, bool *pbRet, vector<BYTE> *pInBuffer, vector<BYTE> *pOutBuffer)
{
	*pbRet = false;
	string strURI = GetURI();
	//string strQuery = GetQuery();

	map<string, string> mapValues = GetAllValues();

	const string strQuery = [this, mapValues]()->string
		{
#ifndef _DEBUG
			const string strTest = GetQuery();
			if (strTest.find(USER_AGENT_3S3S) == -1)
				return strTest;
#endif

			string strRet;
			auto it = mapValues.begin();
			while(mapValues.size() && it != mapValues.end())
			{
				if (it->first == USER_AGENT_3S3S)
				{
					it++;
					continue;
				}

				if (strRet.length()) strRet += "&";

				strRet += it->first;
				if (it->second.length())
					strRet += "=" + it->second;

				it++;
			}
			return strRet;
		}();

	string strHost = GetHost();

	int nPos = strHost.find("."+strName);
	const int nPosBegin = strHost.find(" ");
	
	string strProtocol = "http://";
	int nPosProtocol = strURI.find(strProtocol);

	if (nPosProtocol != 0)
	{
		strProtocol = "https://";
		nPosProtocol = strURI.find(strProtocol);
	}

	if (nPosProtocol == 0)
	{
		if (nPos == -1)
		{
			strHost += "."+strName;
			nPos = strHost.find("."+strName);
		}

		string strRight = strURI.substr(strURI.find(strProtocol)+strProtocol.length());
		
		int nSlash = strRight.find("/");
		if (nSlash == -1)
			strURI = "/";
		else
			strURI = strRight.substr(nSlash);
	}

	if (nPos == -1)
	{
		strHost += "."+strName;
		nPos = strHost.find("."+strName);
	}

#ifdef _DEBUG
	if (strHost.find(" localhost") == 0)
		return false;
#endif

	if (/*(nPos != -1) && */(strHost.find(" www."+strName) == -1) && (strHost.find(" "+strName) == -1) && (nPosBegin+1 <= nPos-1))
	{
		*pbRet = true;

		if (strURI == "/index.ssp")
			strURI = "";

		if (strQuery.length())
			strURI += "?"+strQuery;

		if (!m_Proxy.Continue(strHost.substr(nPosBegin+1, nPos-1), strURI, GetMapNameToValue(), pInBuffer, pOutBuffer, IsSSL(), GetIP(), mapValues))
			*pbRet = false;

		if (m_Proxy.IsStopped())
			Delete();

		return true;
	}
	
	return false;
}

bool ClientForProxy::OnSimpleServerPluginGet(vector<BYTE> *pOutBuffer)
{
	string strURI = GetURI();
	map<string, string> mapValues = GetAllValues();

	auto it = m_ptrPagesGET.begin();
	while (m_ptrPagesGET.size() && it != m_ptrPagesGET.end())
	{
		if (strURI == "/"+it->first || strURI == "/ru/"+it->first || strURI == "/en/"+it->first || mapValues.size())
		{
			const string strBrowserLang = GetBrowserLang();
			if (strURI == "/"+it->first)
			{
				const string strTemp = strURI;
				if (strBrowserLang != "ru")
					strURI = "/en" + strTemp;
				else
					strURI = "/ru" + strTemp;
			}

			if (strURI == "/en/"+it->first)
				it->second->ShowEN(GetAllValues(), GetIP(), HasGzip(), pOutBuffer);
			if (strURI == "/ru/"+it->first)
				it->second->Show(GetAllValues(), GetIP(), HasGzip(), pOutBuffer);

			Delete();
			if (!pOutBuffer->size())
				return false;

			return true;
		}
		it++;
	}

	return false;
}
bool ClientForProxy::OnSimpleServerPluginPost(vector<BYTE> *pInBuffer, vector<BYTE> *pOutBuffer)
{
	string strURI = GetURI();
	
	auto it = m_ptrPagesPOST.begin();
	while (m_ptrPagesPOST.size() && it != m_ptrPagesPOST.end())
	{
		if (strURI == "/"+it->first)
		{
			if (!IsAllRecieved())
				return true;
		
			it->second->Show(GetAllValues(), GetIP(), false, pOutBuffer);

			Delete();

			if (pOutBuffer->size())
				return true;
		}
		it++;
	}

#if 0
	if (strURI == "/register.ssp")
	{
		if (!IsAllRecieved())
			return true;
		
		m_pageRegister.Show(GetAllValues(), GetIP(), pOutBuffer);

		Delete();

		if (pOutBuffer->size())
			return true;
	}
	if (strURI == "/make_short_url.ssp")
	{
		if (!IsAllRecieved())
			return true;
		
		m_pageMakeUrl.Show(GetAllValues(), GetIP(), pOutBuffer);

		Delete();

		if (pOutBuffer->size())
			return true;

		return false;
	}
#endif
	/*map<string, string> mapValues = GetAllValues();
	string strMethod = (mapValues.find("Method") == mapValues.end()) ? "" : mapValues.at("Method");
	string strFolder = (mapValues.find(strMethod) == mapValues.end()) ? "" : mapValues.at(strMethod);

	if (strFolder.find("http://") == 0)
	{
		if (mapValues.find("Authorization") == mapValues.end())
		{
			ostringstream str;
			str << 
				"HTTP/1.1 401 Not Authorized\r\n" << 
				"WWW-Authenticate: Basic realm=\"insert realm\"\r\n" <<
				"\r\n";

			string strResponce = str.str();
			pOutBuffer->resize(strResponce.length());
			memcpy(&((*pOutBuffer)[0]), strResponce.c_str(), strResponce.size());
			Delete();
		}
		else
		{
		}
		return true;
	}*/

	bool bRet;
	for (size_t n=0; n<m_vProxyDNS.size(); n++)
	{
		if (StartProxy(m_vProxyDNS[n], &bRet, pInBuffer, pOutBuffer))
			return bRet;
	}

	return false;
}
