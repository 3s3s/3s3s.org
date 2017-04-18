#ifndef _SSP_PROXY
#define _SSP_PROXY
#include "../curl_helper.h"
#include "../StartupInfo.h"


#ifndef BYTE
typedef unsigned char BYTE;
#endif

using namespace std;

class CSSPProxySiteInfo
{
public:
	time_t m_timeCurrent;
	string m_strURI_utf8, m_strTitle_utf8, m_strDescription_utf8;
	string m_strUserIP, m_strLanguage;

	CSSPProxySiteInfo(time_t tmCurrent, string strURI_utf8, string strTitle_utf8, string strDescription_utf8, string strUserIP, string strLanguage) :
		m_timeCurrent(tmCurrent), m_strURI_utf8(strURI_utf8), m_strTitle_utf8(strTitle_utf8), m_strDescription_utf8(strDescription_utf8),
		m_strUserIP(strUserIP), m_strLanguage(strLanguage)
	{
	}
};


class CSSPProxy
{
	//static vector<startup::CRegisteredDNS> m_vRegisteredDNS;
	//static void UpdateRegisteredDNS();

	void AddProxyADScript();

	string m_strProxy, m_strCurrentDNS;
public:
	string GetCurrentDNS() const { return m_strCurrentDNS; }
	class CResolvedIP
	{
	public:
		string m_strIP;
		time_t m_tmTime;

		CResolvedIP(const string strIP) : m_strIP(strIP), m_tmTime(time(0)) {}
	};
	string m_strHost;
private:
	bool m_bHaveTagHTML, m_bHaveContentType, m_bInProxyMode, m_bInProxyMode2;
	string m_strCookies, m_strUserAgent;

	static void UpdateUserSitesTable();


	string m_strDelHost, m_strOptHost;
	struct curl_slist *m_host, *m_hostDel;

	void AddResolvedDNS(CURL *pCurl);
public:
	static string ChangeContentType(const string strPath, const string strOrigin)
	{
		const string strEncodding = "charset=utf-8\r\n";
		if ((strPath.rfind(".htm") == strPath.length()-4) || (strPath.rfind(".html") == strPath.length()-5) ||
			(!strOrigin.length()))
		{
			if (strOrigin.find("text/html") == -1)
				return "Content-Type: text/html; " + strEncodding;
		}
		else if (strPath.rfind(".txt") == strPath.length()-4)
		{
			if (strOrigin.find("text/plain") == -1)
				return "Content-Type: text/plain " + strEncodding;
		}
		else if (strPath.rfind(".css") == strPath.length()-4)
		{
			if (strOrigin.find("text/css") == -1)
				return "Content-Type: text/css; " + strEncodding;
		}
		else if (strPath.rfind(".xml") == strPath.length()-4)
		{
			if (strOrigin.find("text/xml") == -1)
				return "Content-Type: text/xml; " + strEncodding;
		}
		else if (strPath.rfind(".js") == strPath.length()-3)
		{
			if (strOrigin.find("application/javascript") == -1)
				return "Content-Type: application/javascript; " + strEncodding;
		}

		return strOrigin;
	}

/*	static const string ChangeLocation2(const string strLocation, const string strProto, const string strSep="\\/")
	{
		string strProtocol = strProto;
		int nPos = strLocation.find(strProtocol);
		
		const string strRight = strLocation.substr(nPos+strProtocol.length());

		int nSlash = strRight.find(strSep);
		if (nSlash == strRight.npos || nSlash > strRight.find("'"))
			nSlash = strRight.find("'");
		if (nSlash == strRight.npos || nSlash > strRight.find("\""))
			nSlash = strRight.find("\"");
		if (nSlash == strRight.npos || nSlash > strRight.find("\\"))
			nSlash = strRight.find("\\");
		if (nSlash == strRight.npos || nSlash > strRight.find(" "))
			nSlash = strRight.find(" ");
		if (nSlash == strRight.npos || nSlash > strRight.find("<"))
			nSlash = strRight.find("<");
		if (nSlash == strRight.npos)
			nSlash = strRight.find("\r\n");

		if (nSlash == strRight.npos)
			return strLocation;

		if (strRight.substr(0, nSlash).find("." DNS_NAME) != strRight.npos)
			return strLocation;
		if (strRight.substr(0, nSlash).find(".") == strRight.npos)
		{
			DEBUG_LOG("ChangeLocation failed strRight(0, %i)=%s", nSlash, strRight.substr(0, nSlash).c_str());
			return strLocation;
		}
		
		string strHost = strRight.substr(0, nSlash) + "." DNS_NAME;

		if (!IsValidHostName(strHost))
			return strLocation;

		string strTmp = strLocation.substr(0, nPos+strProtocol.length()) + strHost + strRight.substr(nSlash);
		return strLocation.substr(0, nPos+strProtocol.length()) + strHost + strRight.substr(nSlash);
	}*/

	static const string ChangeLocation(const string strLocation, const string strProto, const string strSep="/")
	{
		string strProtocol = strProto;
		int nPos = strLocation.find(strProtocol);

		const string strRight = strLocation.substr(nPos+strProtocol.length());

		//int nPlus = 
		int nSlash = strRight.find(strSep);
		if (strSep == "%2F")
		{
			int nSlash0 = strRight.find("&");
			if (nSlash != strRight.npos && nSlash0 != strRight.npos && nSlash0 < nSlash)
				nSlash = nSlash0;
			else if (nSlash == strRight.npos && nSlash0 != strRight.npos)
				nSlash = nSlash0;
		}

		if (strProto.find("(//") != -1)
		{
			if (nSlash == strRight.npos || nSlash > strRight.find(")"))
				nSlash = strRight.find(")");
		}
		if (nSlash == strRight.npos || nSlash > strRight.find("'"))
			nSlash = strRight.find("'");
		if (nSlash == strRight.npos || nSlash > strRight.find("\""))
			nSlash = strRight.find("\"");
		if (nSlash == strRight.npos || nSlash > strRight.find("\\"))
			nSlash = strRight.find("\\");
		if (nSlash == strRight.npos || nSlash > strRight.find(" "))
			nSlash = strRight.find(" ");
		if (nSlash == strRight.npos || nSlash > strRight.find("<"))
			nSlash = strRight.find("<");
		if (nSlash == strRight.npos)
			nSlash = strRight.find("\r\n");

		if ((nSlash == strRight.npos) || (nSlash == 0))
			return strLocation;

		if (strRight.substr(0, nSlash).find(string(".") + DNS_NAME) != strRight.npos)
			return strLocation;
		if (strRight.substr(0, nSlash).find(".") == strRight.npos)
		{
			DEBUG_LOG("ChangeLocation failed strRight(0, %i)=%s", nSlash, strRight.substr(0, nSlash).c_str());
			return strLocation;
		}
		if (strRight[nSlash-1] == '.') // for example: "http://www."
			return strLocation;

		string strHost = strRight.substr(0, nSlash) + string(".") + DNS_NAME;

		if (!IsValidHostName(strHost))
			return strLocation;

		return strLocation.substr(0, nPos+strProtocol.length()) + strHost + strRight.substr(nSlash);
	}

private:
	bool m_bIsStarted, m_bChunked, m_bDone, m_bFirstBodySended, m_bTextHtml, m_bTextCSS, m_bApplicationJS, m_bGZIP, m_bUTF8, m_bIsLocation;
	string m_strURL, m_strCookie, m_strResponceCode, m_strPageTitle, m_strPageDescription, m_strUserIP, m_strLanguage;
	CURL *m_pCurl;
	time_t m_tmLastTime;

	string m_strReadedHeader;

	vector<string> m_vClientHeaders;
	vector<BYTE> m_vCache;
	void InjectScript(const string strTagName);
	void InjectFooter(vector<BYTE> *pReadedBytes);
	void SavePageInfo(vector<BYTE> *pReadedBytes);

	//string m_strInjectString;

	static const string AddContentLength(const string &strHeader, const int nInjectedLength);
	bool Redirect(vector<BYTE> *pOutBuffer, const string strLocation) const;
	const string GetLocation(const string strHost) const;

	//vector<unsigned char> m_vStartInPostBuffer;

	void ModifyURLs(vector<unsigned char> *pReadedBytes);

	void UpdateCache(const char *pBuffer, const size_t nSize, vector<BYTE> *pOutBuffer, bool bFlush = false);

	void OnAllProxyDone();
	
	static bool IsValidHostName(string &strHost)
	{
		if (strHost.find("<") != strHost.npos || strHost.find(">") != strHost.npos)
			return false;
		return true;
	}

	static const string DeleteSuffix(const string strIn, const string strSuffix = string(".") + DNS_NAME)
	{
		string strRet = strIn;
		int nPos = strRet.find(strSuffix);
		while(nPos != strRet.npos)
		{
			string strTemp = strRet.substr(0, nPos) + strRet.substr(nPos+strSuffix.length());
			strRet = strTemp;
			nPos = strRet.find(strSuffix);
		}
		return strRet;
	}
	static const string ChangeDomain(const string strCookie, bool bIsSSL)
	{
		int nPosDomain = strCookie.find("domain=");
		if (nPosDomain == strCookie.npos)
		{
			nPosDomain = strCookie.find("Domain=");
			if (nPosDomain == strCookie.npos)
				return strCookie;
		}

		int nEnd = strCookie.substr(nPosDomain+7).find(";");
		if (nEnd == strCookie.npos)
			nEnd = strCookie.substr(nPosDomain+7).find("\r\n");

		const string strRight =strCookie.substr(nPosDomain+7+nEnd);
		const string strLeft = bIsSSL?".h_t_t_p_s":"";
		const string strMiddle = (nEnd>0)?strCookie.substr(nPosDomain+7, nEnd):"";

		return strCookie.substr(0, nPosDomain+7) + /*strLeft +*/ strMiddle + string(".") + DNS_NAME + strRight;
	}

	/*static const string ReplaceTxt(const string &strAllOriginText, const string strForReplace, const string strNew)
	{
		string strRet = strAllOriginText;

		int nPos = strRet.find(strForReplace);
		while (nPos != strRet.npos)
		{
			string strTemp = strRet;
			strRet = strTemp.substr(0, nPos+1) + strNew + strTemp.substr(nPos+1+strForReplace.length());

			nPos = strRet.find(strForReplace, nPos+2+strNew.length());
		}


		return strRet;
	}*/

	static const string ChangeSecure(const string strCookie)
	{
		int nPosSecure = strCookie.find(" Secure;");
		if (nPosSecure == strCookie.npos)
		{
			nPosSecure = strCookie.find(" Secure\r");
			if (nPosSecure == strCookie.npos)
			{
				nPosSecure = strCookie.find(" secure;");
				if (nPosSecure == strCookie.npos)
				{
					nPosSecure = strCookie.find(" secure\r");
					if (nPosSecure == strCookie.npos)
						return strCookie;
				}
			}
		}

		return strCookie.substr(0, nPosSecure) + strCookie.substr(nPosSecure+7);
	}
	static const string ChangeCookie(const string strCookie, bool bIsSSL)
	{
		return ChangeSecure(ChangeDomain(strCookie, bIsSSL));
	}

	const string GetLocationHost(const string strHost, const string strLocation) const
	{	
		string strProtocol = "http://";
		int nPos = strLocation.find(strProtocol);
		if (nPos == -1)
		{
			strProtocol = "https://";
			nPos = strLocation.find(strProtocol);
			if (nPos == -1)
				return strHost;
		}

		const string strRight = strLocation.substr(nPos+strProtocol.length());
		const int nSlash = strRight.find("/");
		if (nSlash == -1)
			return strRight;

		return strRight.substr(0, nSlash);
	}
	static const string ChangeReferer(const string strReferer0)
	{
		const string strReferer = [strReferer0]() -> string
			{
				const int nPos = strReferer0.find("http://h_t_t_p_s.");
				if (nPos == strReferer0.npos)
					return strReferer0;
				
				return strReferer0.substr(0, nPos) + "https://" + strReferer0.substr(nPos+17);
			}();

		string strProtocol = "http://";
		int nPos = strReferer.find(strProtocol);
		if (nPos == strReferer.npos)
		{
			strProtocol = "https://";
			nPos = strReferer.find(strProtocol);
			if (nPos == strReferer.npos)
				return strReferer;
		}

		const string strRight = strReferer.substr(nPos+strProtocol.length());

		int nSlash = strRight.find("/");
		if (nSlash == -1 ||  nSlash > strRight.find("\r\n"))
			nSlash = strRight.find("\r\n");
		if (nSlash == -1)
			nSlash = strRight.length()-1;
		if (nSlash == -1)
			return strReferer;

		string strHost = strRight.substr(0, nSlash);
		int nPos3s3s = strHost.find(string(".") + DNS_NAME);
		if (nPos3s3s != strHost.npos)
		{
			string strTmp = strHost.substr(0, nPos3s3s);
			strHost = strTmp;
		}

		const string strRet = strReferer.substr(0, nPos+strProtocol.length()) + strHost + strRight.substr(nSlash);
		return strRet;
	}

	void SetClientHeaders(const map<string, string> &mapHeaders, const string strHost, const string strLocation)
	{
		const string strRealHost = GetLocationHost(strHost, strLocation);
		DEBUG_LOG("SetClientHeaders Client: %s   strRealHost = %s", m_strUserIP.c_str(), strRealHost.c_str());

		DNS_NAME = m_strCurrentDNS;

		string strRet;
		for (auto it = mapHeaders.begin(); it != mapHeaders.end(); ++it)
		{
			/*if (it->first == "Cookie")
			{
				m_strCookie = strHeader;
				continue;
			}*/
			if (it->first == "Host" || it->first == "host")
			{
				m_vClientHeaders.push_back(it->first + ": " + strRealHost);
				continue;
			}


			if (it->first.find("Cache-Control") != -1 || it->first.find("cache-control") != -1)
				continue;
			//if (it->first.find("Expect") != -1)
			//	continue;
			//if (it->first.find("Pragma") != -1)
			//	continue;
			if (it->first.find("Accept-Language") != -1 || it->first.find("accept-language") != -1)
				continue;
			if (it->first.find("GET") != -1)
				continue;
			if (it->first.find("POST") != -1)
				continue;
			if (it->first.find("Method") != -1)
				continue;

			string strSecond = it->second;
			if ((strSecond.find(string(".") + m_strCurrentDNS) != -1) && (strSecond.find("://") != -1))
			//if (it->first.find("Referer") != -1)
				strSecond = ChangeReferer(it->second);
			if (it->first.find("Cookie") != -1 || it->first.find("cookie") != -1)
			{
				if (strHost.find("www.youtube.com") != -1)
					continue;
				strSecond = DeleteSuffix(it->second);
			}
			if (it->first.find("Accept-Encoding") != -1 || it->first.find("accept-encoding") != -1)
				strSecond = "no";


			if (m_strUserAgent.length() && (it->first.find("User-Agent") != -1 || it->first.find("user-agent") != -1) )
				strSecond = m_strUserAgent;
			
			m_vClientHeaders.push_back(it->first + ": " + strSecond);
		}
		//m_vClientHeaders.push_back("Cache-Control: no-store, no-cache, must-revalidate");
		//m_vClientHeaders.push_back("Pragma: no-cache");
		m_vClientHeaders.push_back("Accept-Language: en-US");
	}

	static size_t CURL_Header( void *ptr, size_t size, size_t nmemb, void *userdata);
	bool FlushHeader(vector<BYTE> *pOutBuffer);
	bool FlushBody(vector<BYTE> *pOutBuffer);
public:
	string m_strContentLength;
	unordered_map<string, string> m_mapInjectHTML;
	CSSPProxy() : m_bIsStarted(false), m_bChunked(false), m_bDone(false), 
		m_bFirstBodySended(false), m_bTextHtml(false), m_bGZIP(false),  m_pCurl(NULL), m_bTextCSS(false), 
		m_bApplicationJS(false), m_tmLastTime(time(NULL)), m_bUTF8(false),
		m_bInProxyMode(false), m_bInProxyMode2(false), m_host(NULL), m_hostDel(NULL)
	{
		//UpdateRegisteredDNS();

		m_bHaveContentType = false;
		m_bHaveTagHTML = true;
		m_bIsLocation = false;

		ReadInjectHTML("inject.html", "head");
		ReadInjectHTML("inject.html", "body");

	}

	void ReadInjectHTML(const string strFilePath, const string strTagName)
	{
		FILE *pFile = fopen(strFilePath.c_str(), "rb");
		if (!pFile)
			return;

		string strContent;
		while(!feof(pFile))
		{
			char szBuff[2];
			if (!fread(szBuff, 1, 1, pFile))
				break;

			strContent += szBuff[0];
		}

		fclose(pFile);

		int nPosStart = strContent.find("<"+strTagName+">");
		if (nPosStart == -1)
			return;

		nPosStart += strTagName.length()+2;
		string strRight = strContent.substr(nPosStart);

		int nPosEnd = strRight.find("</"+strTagName+">");
		if (nPosEnd == -1)
			return;

		const string strInject = strRight.substr(0, nPosEnd);
		m_mapInjectHTML[strTagName] = strInject;
	}

	~CSSPProxy();
	bool Continue(const string strHost, const string strURI, const map<string, string> &mapHeaders, vector<BYTE> *pInBuffer, vector<BYTE> *pOutBuffer, const bool bIsSSL, const string strIP, const map<string, string> &mapValues);
	bool IsStopped() const;
};

#endif