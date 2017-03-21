#include <stdio.h>
#include <vector>
#include <string.h>

#include "Proxy.h"
#include "../html_framework.h"
#include "../utils/orm.h"

extern curl::CUrl gCURL;
extern set<string> g_vWhiteListedIP;

//vector<startup::CRegisteredDNS> CSSPProxy::m_vRegisteredDNS;
vector<CSSPProxySiteInfo> m_gLastProxySites;

set<string> m_gSetIPandURLs;

unordered_map<string, shared_ptr<CSSPProxy::CResolvedIP> > g_mapHostToIP;


using namespace html;
using namespace std;

CSSPProxy::~CSSPProxy()
{
	if (m_strURL.length())
	{
		DEBUG_LOG("Destroy Proxy for url=%s", m_strURL.c_str());
	}
	if (m_pCurl)
		gCURL.DeleteHandle(m_pCurl);

	if (m_host)
		curl_slist_free_all(m_host);
	if (m_hostDel)
		curl_slist_free_all(m_hostDel);
}

bool CSSPProxy::IsStopped() const
{
	/*if (m_bTextHtml && (m_strURL == "http://e1.ru/") && m_bDone)
	{
		DEBUG_LOG("DONE PROXY !");
	}*/
	return m_bDone;
}

/*void CSSPProxy::UpdateRegisteredDNS()
{
	if (m_vRegisteredDNS.size())
		return;

	startup::CRegisteredDNS langtest("langtest.ru", "193.107.236.167", 8085, 8443);
	startup::CRegisteredDNS multicoins("multicoins.org", "104.236.180.129", 8088, 9443);

	/*for (size_t n=0; n<m_vRegisteredDNS.size(); n++)
	{
		if (m_vRegisteredDNS[n].DNS() == langtest.DNS())
			return;
		if (m_vRegisteredDNS[n].DNS() == multicoins.DNS())
			return;
	}*/
	/*m_vRegisteredDNS.push_back(langtest);
	m_vRegisteredDNS.push_back(multicoins);
}*/

void CSSPProxy::InjectScript(const string strTagName)
{
	if (m_bGZIP)
	{
		DEBUG_LOG("ERROR: InjectScript gzip");
		return;
	}
	if (m_mapInjectHTML.find(strTagName) == m_mapInjectHTML.end())
		return;

	vector<BYTE> *pReadedBytes = gCURL.GetReadedBytes(m_pCurl);
	pReadedBytes->push_back(0);

	string strHTML;
	vector<BYTE> vGZIP;
	if (!m_bGZIP)
		strHTML = (const char *)&(*pReadedBytes)[0];
	else
	{
		utils::gzipInflate(*pReadedBytes, vGZIP);
		strHTML = (const char *)&vGZIP[0];
	}
	if ((utils::ci_find_substr( strHTML, "</html>" ) == -1) &&
		(utils::ci_find_substr( strHTML, "</script>" ) == -1))
	{
		pReadedBytes->pop_back();
		return;
	}
	
	int nPosHTMLStart = utils::ci_find_substr( strHTML, "<html" );
	int nPosHTMLEnd = -1;
	if (nPosHTMLStart != -1)
		nPosHTMLEnd = strHTML.find('>', nPosHTMLStart);
	else
		m_bHaveTagHTML = false;

	int nPosHeadStart = utils::ci_find_substr( strHTML, "<"+strTagName );
	int nPosHeadEnd = -1;
	if (nPosHeadStart != -1)
		nPosHeadEnd = strHTML.find('>', nPosHeadStart);

	if (nPosHTMLEnd < 0)
	{
		nPosHTMLEnd = 0;
		if (nPosHeadEnd == -1)
			nPosHeadEnd = 0;
	}

	if (nPosHeadEnd == -1)
	{
		if (strTagName == "body")
		{
			pReadedBytes->pop_back();
			return;
		}

		string strTemp = strHTML.substr(0, nPosHTMLEnd+1) + "<"+strTagName+"></"+strTagName+">" + strHTML.substr(nPosHTMLEnd+1, strHTML.length()-nPosHTMLEnd-1);
		strHTML = strTemp;
		nPosHeadStart = utils::ci_find_substr( strHTML, "<"+strTagName+">" );
		nPosHeadEnd = nPosHeadStart+5;
	}
		
	const int nHeadStart = (nPosHeadEnd == 0) ? 0 : nPosHeadEnd+1;

	const string strInject = m_mapInjectHTML[strTagName];

	string strTemp = strHTML.substr(0, nHeadStart) + strInject + strHTML.substr(nHeadStart, strHTML.length()-nHeadStart);

	pReadedBytes->resize(strTemp.length());
	memcpy(&((*pReadedBytes)[0]), strTemp.c_str(), strTemp.length());
}

void CSSPProxy::ModifyURLs(vector<unsigned char> *pReadedBytes)
{
	pReadedBytes->push_back(0);

	string str = (const char *)&pReadedBytes->at(0);
	if (str.length() != pReadedBytes->size()-1)
	{
		pReadedBytes->pop_back();
		return;
	}

	string strNew = str;
	
	static const string strHTTP[] = {"'http://", "\"http://", "'ftp://", "\"ftp://", "'//", "url(//", "\"//", "=http://"};
	for (int n=0; n<8; n++)
	{
		int nPos = strNew.find(strHTTP[n]);
		while (nPos != strNew.npos)
		{
			string strTemp = strNew;
			strNew = strTemp.substr(0, nPos);

#ifdef _DEBUG
			const string strTmp0 = strTemp.substr(nPos-10);
#endif
			
			const string strTmp = (nPos > 6)?strNew.substr(nPos-5, 5):"";
			if (strTmp != "data=")
				strNew += ChangeLocation(strTemp.substr(nPos), strHTTP[n]);
			else
				strNew += strTemp.substr(nPos);

			nPos = strNew.find(strHTTP[n], nPos+1);
		}
	}

	if (m_strURL.find("www.youtube.com") == -1)
	{
		static const string strHTTP2[] = {"'http:\\/\\/", "\"http:\\/\\/"};
		for (int n=0; n<2; n++)
		{
			int nPos = strNew.find(strHTTP2[n]);
			while (nPos != strNew.npos)
			{
				string strTemp = strNew;
				strNew = strTemp.substr(0, nPos);
				strNew += ChangeLocation(strTemp.substr(nPos), strHTTP2[n], "\\/");
				nPos = strNew.find(strHTTP2[n], nPos+1);
			}
		}

		static const string strHTTP3[] = {"http%3A%2F%2F", "https%3A%2F%2F"};
		for (int n=0; n<2; n++)
		{
			int nPos = strNew.find(strHTTP3[n]);
			while (nPos != strNew.npos)
			{
				string strTemp = strNew;
				strNew = strTemp.substr(0, nPos);
				strNew += ChangeLocation(strTemp.substr(nPos), strHTTP3[n], "%2F");
				nPos = strNew.find(strHTTP3[n], nPos+1);
			}
		}
	}
	
	

	if (m_strURL.find("www.youtube.com") == -1)
	{
		static const string strHTTPS[] = {"'https://", "\"https://", "=https://"};
		for (int n=0; n<3; n++)
		{
			int nPos = strNew.find(strHTTPS[n]);
			while (nPos != strNew.npos)
			{
				string strTemp = strNew;
				strNew = strTemp.substr(0, nPos+1);
				strNew += "http://h_t_t_p_s." + ChangeLocation(strTemp.substr(nPos), strHTTPS[n]).substr(strHTTPS[n].length());
				nPos = strNew.find(strHTTPS[n], nPos+2);
			}
		}

	//if (m_strURL.find("www.youtube.com") == -1)
	//{
		static const string strHTTPS2[] = {"'https:\\/\\/", "\"https:\\/\\/"};
		for (int n=0; n<2; n++)
		{
			int nPos = strNew.find(strHTTPS2[n]);
			while (nPos != strNew.npos)
			{
				string strTemp = strNew;
				strNew = strTemp.substr(0, nPos+1);
				strNew += "http:\\/\\/h_t_t_p_s." + ChangeLocation(strTemp.substr(nPos), strHTTPS2[n], "\\/").substr(strHTTPS2[n].length());
				nPos = strNew.find(strHTTPS2[n], nPos+2);
			}
		}
	}
	//ReplaceTxt(strNew, "document.domain", "document.domain3s3s");
	if (m_strURL.find("www.youtube.com") != -1)
	{
		static const string strYT_HTTPS[] = {"\"ssl\":\"1\"", "ssl=1"};
		static const string strYT_HTTP[] = {"\"ssl\":\"0\"", "ssl=0"};

		for (int n=0; n<2; n++)
		{
			int nPos = strNew.find(strYT_HTTPS[n]);
			if (nPos != strNew.npos)
			{
				string strTemp = strNew;
				strNew = strTemp.substr(0, nPos+1);
				strNew += strYT_HTTP[n] + strTemp.substr(nPos+strYT_HTTPS[n].length());
				//nPos = strNew.find(strHTTPS[n], nPos+2);
			}
		}
		/*int nPos = strNew.find("\"ssl\":\"1\"");
		if (nPos != strNew.npos)
		{
			string strTemp = strNew;
			strNew = strTemp.substr(0, nPos+1);
			strNew += "ssl\":\"0\"" + strTemp.substr(nPos+9);
			//nPos = strNew.find(strHTTPS[n], nPos+2);
		}*/
	}

	if (true)
	{
		int nPos = strNew.find("location");
		while (nPos != strNew.npos)
		{
			string strTemp = strNew;
			strNew = strTemp.substr(0, nPos);
			strNew += "location_" + strTemp.substr(nPos+8);
			nPos = strNew.find("location", nPos+2);
		}
	}
	

	pReadedBytes->resize(strNew.length());
	memcpy(&pReadedBytes->at(0), strNew.c_str(), strNew.length());
}

void CSSPProxy::SavePageInfo(vector<BYTE> *pReadedBytes)
{
	if (m_strPageTitle.length())
		return;

	DEBUG_LOG("SavePageInfo start");

	using namespace html;

	const string strPage = (const char *)&pReadedBytes->at(0);

	m_strPageTitle = html::utils::GetTagContent(strPage, "title");
	DEBUG_LOG("GetTagContent return");
	
	html::utils::Replace(m_strPageTitle, 0, "\r", "");
	html::utils::Replace(m_strPageTitle, 0, "\n", "");
	html::utils::Replace(m_strPageTitle, 0, "\t", "");

	vector<string> tagsMeta = html::utils::GetTagsArray(strPage, "meta");
	DEBUG_LOG("GetTagsArray size=%i", tagsMeta.size());

	for (size_t n=0; n<tagsMeta.size(); n++)
	{
		const int nPosDescription = html::utils::ci_find_substr(tagsMeta[n], "description");
		if (nPosDescription == -1)
			continue;

		const int nDescriptionStart = html::utils::ci_find_substr(tagsMeta[n], "content=");
		if (nDescriptionStart == -1)
			continue;

		const string strQuote = (tagsMeta[n][nDescriptionStart+1] == '\"') ? "'" : "\"";

		const int nDescriptionEnd = html::utils::ci_find_substr(tagsMeta[n], strQuote, nDescriptionStart+9);
		if (nDescriptionEnd == -1)
			continue;

		m_strPageDescription = tagsMeta[n].substr(nDescriptionStart+9, nDescriptionEnd-nDescriptionStart-9);
		break;
	}

	/*const int nDescription = html::utils::ci_find_substr(strPage, "description");
	if (nDescription == -1)
		return;

	const int MetaDescriptionEnd = 

	const string strDescriptionPart = strPage.substr(0, nDescription);
	const int nMetaStart = strDescriptionPart.rfind("<");
	if (nMeta == -1)
		return;

	const string strMetaDescriptionPart = strDescriptionPart.substr(nMeta);

	const int nDescriptionStart = html::utils::ci_find_substr(strMetaDescriptionPart, "content=");
	if (nDescriptionStart == -1)
		return;

	m_strPageDescription = strMetaDescriptionPart.substr(nDescriptionStart+8);
	html::utils::Replace(m_strPageTitle, 0, "\"", "");
	html::utils::Replace(m_strPageTitle, 0, "\'", "");*/
}

void CSSPProxy::InjectFooter(vector<BYTE> *pReadedBytes)
{
	return;
	if (m_bGZIP)
	{
		DEBUG_LOG("InjectFooter gzip");
		return;
	}
	if (pReadedBytes->size() < 6)
		return;
	
	DEBUG_LOG("InjectFooter start");

	pReadedBytes->push_back(0);

	string strHTML;
	vector<BYTE> vGZIP;
	if (!m_bGZIP)
		strHTML = (const char *)&(*pReadedBytes)[0];
	else
	{
		DEBUG_LOG("InjectFooter gzip");
		utils::gzipInflate(*pReadedBytes, vGZIP);
		strHTML = (const char *)&vGZIP[0];
	}
	
	int nPosInjected = strHTML.find(string("Anonymoused by www.") + m_strCurrentDNS);
	DEBUG_LOG("InjectFooter find1 - ok");
	if (nPosInjected > 0)
	{
		pReadedBytes->pop_back();
		return;
	}

	int nPosBODYEnd = utils::ci_find_substr( strHTML, "</body" );
	DEBUG_LOG("InjectFooter find2 - ok");
	if (nPosBODYEnd > 0)
	{
		string strInjectString = string("<div style='display:block; width:100%; height: 10px'><center><a href='http://www.") + m_strCurrentDNS + "'>Anonymoused by www." + DNS_NAME + "</s></center></div>";
		string strTemp = strHTML.substr(0, nPosBODYEnd) + strInjectString + strHTML.substr(nPosBODYEnd, strHTML.length()-nPosBODYEnd);

		pReadedBytes->resize(strTemp.length());
		memcpy(&((*pReadedBytes)[0]), strTemp.c_str(), pReadedBytes->size());
	}
	else
		pReadedBytes->pop_back();

}

void CSSPProxy::AddProxyADScript()
{
	//if (!m_bInProxyMode)
		return;

	auto it = m_mapInjectHTML.find("head");
	if (it == m_mapInjectHTML.end())
		m_mapInjectHTML["head"] = "";

	m_mapInjectHTML["head"] +=
		"\n<script type=\"text/javascript\" src=\"http://coinurl.com/script/jquery-latest.min.js\"></script>"
		"\n<script type=\"text/javascript\">"
		"\n$(function(){for(var r=Array(),e=Array(),t=\"8e64ba51d55e57945dc704f6ca55ad5c\",f=\"http://cur.lv/redirect.php?id=\"+t+\"&url=\",a=$(\"a[href^='http']\"),n=0;n<a.length;n++){for(var i=$(a[n]).attr(\"href\"),h=!1,c=0;c<e.length;c++)if(-1!=i.indexOf(e[c])){h=!0;break}if(!h){if(r.length>0){for(var d=!1,c=0;c<r.length;c++)if(-1!=i.indexOf(r[c])){d=!0;break}if(!d)continue}$(a[n]).attr(\"href\",f+encodeURIComponent(i))}}});"
		"\n</script>";
}

bool CSSPProxy::FlushBody(vector<BYTE> *pOutBuffer)
{
	DEBUG_LOG("FlushBody start for Client=%s    url=%s", m_strUserIP.c_str(), m_strURL.c_str());
	m_tmLastTime = time(NULL);
	if (!m_bFirstBodySended && m_bTextHtml)
	{
		m_bFirstBodySended = true;

		if (!m_bInProxyMode2)
		{
			if ((m_strURL.find("play.google.com") == -1) && (m_strURL.find("raw.githubusercontent.com/3s3s") == -1) &&
				(m_strURL.find("3s3s.github.io/github.io") == -1))
			{
				AddProxyADScript();
				InjectScript("head");
				InjectScript("body");
			}
		}
	}

	DEBUG_LOG("FlushBody start 1");

	ostringstream strChunkStart;
	string strChunk;
	string strEnd;
	
	vector<BYTE> *pReadedBytes = gCURL.GetReadedBytes(m_pCurl);

	DEBUG_LOG("FlushBody start 2");

	/*bool bJS = false;
	if (m_strURL.find(".js") == m_strURL.length()-3)
	{
		//if (m_strURL.find("s0.grani") == -1)
		//{
		//	DEBUG_LOG("(m_strURL.find(\"s0.grani\") == -1)");
			bJS = true;
		//}
	}*/

	if ((m_bTextHtml || m_bTextCSS || m_bApplicationJS) && (m_strURL.find("raw.githubusercontent.com/3s3s") == -1) &&
		(m_strURL.find("3s3s.github.io/github.io") == -1))
	{
//ifdef _DEBUG
		/*if (m_strURL.find(".js") == m_strURL.length()-3)
		{
			__asm int 3;
		}*/
//#endif
		DEBUG_LOG("ModifyURLs");
		if (!m_bInProxyMode && !m_bInProxyMode2)
		{
			ModifyURLs(pReadedBytes);
			DEBUG_LOG("InjectFooter");
			InjectFooter(pReadedBytes);
		}

		if (m_bTextHtml)
			SavePageInfo(pReadedBytes);
	}

	size_t nSendSize = pReadedBytes->size();
	//if (nSendSize > 10000)
	//	nSendSize = 10000;
	DEBUG_LOG("FlushBody start 3");
	
	if (m_bChunked)
	{
		strChunkStart << std::hex << nSendSize << "\r\n";
		strChunk = strChunkStart.str();
		strEnd = "\r\n";
	}

	DEBUG_LOG("FlushBody middle part");

	/*pOutBuffer->resize(strChunk.length()+nSendSize+strEnd.length());
	if (m_bChunked)
		memcpy(&((*pOutBuffer)[0]), strChunk.c_str(), strChunk.length());
	memcpy(&((*pOutBuffer)[strChunk.length()]), &((*pReadedBytes)[0]), nSendSize);
	if (m_bChunked)
		memcpy(&((*pOutBuffer)[strChunk.length()+nSendSize]), strEnd.c_str(), strEnd.length());*/
	pOutBuffer->clear();
	if (m_bChunked)
		UpdateCache(strChunk.c_str(), strChunk.length(), pOutBuffer);
	
	UpdateCache((const char *)&((*pReadedBytes)[0]), nSendSize, pOutBuffer);
	if (m_bChunked)
		UpdateCache(strEnd.c_str(), strEnd.length(), pOutBuffer);

	if (nSendSize < pReadedBytes->size())
	{
		vector<BYTE> temp(pReadedBytes->size()-nSendSize);
		memcpy(&temp[0], &((*pReadedBytes)[nSendSize]), pReadedBytes->size()-nSendSize);
		*pReadedBytes = move(temp);
	}
	else
		pReadedBytes->clear();
	
	DEBUG_LOG("FlushBody end");
	return true;
}
bool CSSPProxy::FlushHeader(vector<BYTE> *pOutBuffer)
{
	string strTemp = m_strReadedHeader;
	int nPos = m_strReadedHeader.find("\r\n");
	DEBUG_LOG("nPos = %i", nPos);

	strTemp = m_strReadedHeader.substr(0, nPos+2) + m_strCookies + m_strReadedHeader.substr(nPos+2);
	m_strReadedHeader = strTemp;

	DEBUG_LOG("FlushHeader for url=%s", m_strURL.c_str());
	DEBUG_LOG("m_bTextHtml = %i; m_bTextCSS = %i, m_bApplicationJS = %i, m_bInProxyMode=%i, m_bInProxyMode2=%i", m_bTextHtml, m_bTextCSS, m_bApplicationJS, m_bInProxyMode, m_bInProxyMode2);
	DEBUG_LOG("m_strReadedHeader = %s; m_strContentLength  = %s", m_strReadedHeader.c_str(), m_strContentLength.c_str());
	//if (m_strContentLength.length())
	//{
	if (!m_bHaveContentType)
	{
		string strCT = ChangeContentType(m_strURL, "");
		if (strCT == "") strCT = "\r\n";
		strTemp = m_strReadedHeader.substr(0, nPos+2) + strCT + m_strReadedHeader.substr(nPos+2);
		m_strReadedHeader = strTemp;
		nPos = m_strReadedHeader.find("\r\n");

		/*if ((m_strURL.rfind(".html") == m_strURL.length()-5) || 
			(m_strURL.rfind(".js") == m_strURL.length()-3) ||
			(m_strURL.rfind(".css") == m_strURL.length()-4))
		{
			m_bUTF8 = true;
			
			if (m_strURL.rfind(".html") == m_strURL.length()-5)
			{
				m_bTextHtml = true;
				strTemp = m_strReadedHeader.substr(0, nPos+2) + "Content-Type: text/html; charset=utf-8\r\n" + m_strReadedHeader.substr(nPos+2);
			}
			else if (m_strURL.rfind(".js") == m_strURL.length()-3)
			{
				strTemp = m_strReadedHeader.substr(0, nPos+2) + "Content-Type: application/javascript; charset=utf-8\r\n" + m_strReadedHeader.substr(nPos+2);
			}
			else if (m_strURL.rfind(".css") == m_strURL.length()-4)
			{
				m_bTextCSS = true;
				strTemp = m_strReadedHeader.substr(0, nPos+2) + "Content-Type: text/css; charset=utf-8\r\n" + m_strReadedHeader.substr(nPos+2);
			}
			DEBUG_LOG("strTemp = %s", strTemp.c_str());
			
			m_strReadedHeader = strTemp;

			nPos = m_strReadedHeader.find("\r\n");
		}*/
	}
		
	if (!m_bInProxyMode && !m_bInProxyMode2 && (m_bTextHtml || m_bTextCSS || m_bApplicationJS))
		{
			strTemp = 
				m_strReadedHeader.substr(0, nPos+2) + 
				"Transfer-Encoding: chunked\r\n" +
				//"Access-Control-Allow-Origin: *\r\n" + 
				m_strReadedHeader.substr(nPos+2);
		}
		else if (m_strContentLength.length())
			strTemp = m_strReadedHeader.substr(0, nPos+2) + m_strContentLength + m_strReadedHeader.substr(nPos+2);
		
		m_strReadedHeader = strTemp;
	//}

	DEBUG_LOG("m_strReadedHeader = %s", m_strReadedHeader.c_str());
	if (m_strReadedHeader.find("Transfer-Encoding: chunked") != -1)
		m_bChunked = true;
	if (m_strReadedHeader.find("Cache-Control:") == m_strReadedHeader.npos)
	{
		string strTemp;
		int nPos = m_strReadedHeader.find("\r\n");
		strTemp = m_strReadedHeader.substr(0, nPos+2) + 
			"Cache-Control: no-store, no-cache, must-revalidate\r\n" +
			"Pragma: no-cache\r\n" +
			m_strReadedHeader.substr(nPos+2);
		m_strReadedHeader = strTemp;
	}

	DEBUG_LOG("Header: %s", m_strReadedHeader.c_str());

	//pOutBuffer->resize(m_strReadedHeader.length());
	//memcpy(&((*pOutBuffer)[0]), m_strReadedHeader.c_str(), m_strReadedHeader.length());
	pOutBuffer->clear();
	UpdateCache(m_strReadedHeader.c_str(), m_strReadedHeader.length(), pOutBuffer, true);
	m_strReadedHeader = "";
	
	return true;
}

const string CSSPProxy::GetLocation(const string strHost) const 
{
	const int nDotPos = strHost.find(".");
	if (nDotPos != strHost.npos && nDotPos != strHost.length()-2)
		return "";

	orm::CTable exist = orm::CTable("LINKS").Where("short=\'%s\' COLLATE NOCASE", strHost.c_str()).GetAllTable();
	if (!exist.GetRowsCount())
		return "";

	string strRet = exist[0]["long"];

	if (strRet.length() < 10)
		return "";
	if ((strRet.find("http://") == -1) && (strRet.find("https://") == -1))
		return "";

	return strRet;
}

bool CSSPProxy::Redirect(vector<BYTE> *pOutBuffer, const string strLocation) const
{
	ostringstream str;
	str << "HTTP/1.1 301 Moved Permanently\r\n"
		<< "Location: "<< strLocation << "\r\n"
		<< "Cache-Control: no-cache\r\n"
		<< "Connection: close\r\n"
		<< "Content-Length: 0\r\n"
		<< "\r\n";

	DEBUG_LOG("Redirect: %s", str.str().c_str());

	string strResponce = str.str();
	pOutBuffer->resize(strResponce.length());
	memcpy(&((*pOutBuffer)[0]), strResponce.c_str(), strResponce.size());
	return true;
}

void CSSPProxy::UpdateCache(const char *pBuffer, const size_t nSize, vector<BYTE> *pOutBuffer, bool bFlush)
{
	if (nSize)
	{
		m_vCache.resize(m_vCache.size()+nSize);
		memcpy(&m_vCache[m_vCache.size()-nSize], pBuffer, nSize);
	}
	/*m_vCache.push_back(0);
	DEBUG_LOG("cache = %s", (const char *)&m_vCache[0]);
	m_vCache.pop_back();*/

	if (m_vCache.size() && (m_vCache.size() > 0xfff || bFlush))
	{
		const int nOldSize = pOutBuffer->size();

		pOutBuffer->resize(pOutBuffer->size()+m_vCache.size());
		memcpy(&pOutBuffer->at(nOldSize), &m_vCache[0], m_vCache.size());
		m_vCache.clear();

		/*pOutBuffer->push_back(0);
		DEBUG_LOG("fluched cache = %s", (const char *)&pOutBuffer->at(0));
		pOutBuffer->pop_back();*/
	}
}

void CSSPProxy::UpdateUserSitesTable()
{
	DEBUG_LOG("UpdateUserSitesTable m_gLastProxySites.size()=%i", m_gLastProxySites.size());

	static time_t tmDayAgo = time(0);

	const time_t tmCurr = time(0);

	if ((tmCurr - tmDayAgo >= 24*3600) || m_gSetIPandURLs.size() > 100000)
	{
		m_gSetIPandURLs.clear();
		tmDayAgo = tmCurr;
	}

	
	orm::CTable::ExecuteSQL("BEGIN TRANSACTION");

	orm::CTable::ExecuteSQL("DELETE FROM LINKS WHERE time IS NOT NULL AND status='temp' AND time > 0 AND time+43200 < %i", (int)tmCurr);
	orm::CTable::ExecuteSQL("DELETE FROM LINKS WHERE status='delete'");
	//orm::CTable::ExecuteSQL("VACUUM");

	for (size_t nIndex = 0; nIndex<m_gLastProxySites.size(); nIndex++)
	{

		//orm::CTable::CreateTable("UsersSites", "(url TEXT, title TEXT, description TEXT, UserIP TEXT, time INTEGER)");
/*		const orm::CTable oldTable =
			orm::CTable("UsersSites").Where("url='%s' AND UserIP='%s' AND time>%s", 
			m_gLastProxySites[nIndex].m_strURI_utf8.c_str(), 
			orm::utils::ReplaceQuotes(m_gLastProxySites[nIndex].m_strUserIP).c_str(), 
			to_string(tmCurr-24*3600).c_str()).GetAllTable();
	
		if (oldTable.GetRowsCount() != 0)
			continue;*/

		const string strIPandURL = m_gLastProxySites[nIndex].m_strUserIP+"-->"+m_gLastProxySites[nIndex].m_strURI_utf8;

		if (m_gSetIPandURLs.find(strIPandURL) != m_gSetIPandURLs.end())
			continue;

		m_gSetIPandURLs.insert(strIPandURL);

		orm::CTable("UsersSites").Insert("'%s', '%s', '%s', '%s', %s, 0, '%s'",
			m_gLastProxySites[nIndex].m_strURI_utf8.c_str(),
			m_gLastProxySites[nIndex].m_strTitle_utf8.c_str(),
			m_gLastProxySites[nIndex].m_strDescription_utf8.c_str(),
			orm::utils::ReplaceQuotes(m_gLastProxySites[nIndex].m_strUserIP).c_str(),
			to_string(tmCurr).c_str(),
			m_gLastProxySites[nIndex].m_strLanguage.c_str());
	}

	orm::CTable::ExecuteSQL("COMMIT TRANSACTION");

	m_gLastProxySites.clear();
}

void CSSPProxy::OnAllProxyDone()
{
	gCURL.DeleteHandle(m_pCurl);
	m_pCurl = NULL;
	m_bDone = true;
	DEBUG_LOG("ALL DONE! ip=%s, url=%s, m_strPageTitle.length()=%i, m_bIsLocation=%i, m_bInProxyMode=%i, m_bInProxyMode2=%i", 
		m_strUserIP.c_str(), m_strURL.c_str(), m_strPageTitle.length(), m_bIsLocation, m_bInProxyMode, m_bInProxyMode2);
	
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	if (g_vWhiteListedIP.find(m_strUserIP) == g_vWhiteListedIP.end())
		return;

	if (!m_strPageTitle.length() || m_bIsLocation || m_bInProxyMode || m_bInProxyMode2)
		return;

	if (m_strPageTitle.length() >= m_strPageDescription.length())
		return;

	vector<string> vBanURLs;
	vBanURLs.push_back("github.com");
	/*vBanURLs.push_back("www.dziennik.pl");
	vBanURLs.push_back("www.slideshare.net");
	vBanURLs.push_back("www.engadget.com");
	vBanURLs.push_back("ameblo.jp");*/
	
	for (size_t n=0; n<vBanURLs.size(); n++)
	{
		if (m_strURL.find(vBanURLs[n]) != -1)
			return;
	}

	const string strURI_utf8 = html::utils::ConvertString(orm::utils::ReplaceQuotes(m_strURL).c_str());
	const string strTitle_utf8 = m_bUTF8 ? orm::utils::ReplaceQuotes(m_strPageTitle).c_str() : html::utils::ConvertString(orm::utils::ReplaceQuotes(m_strPageTitle).c_str());
	const string strDescription_utf8 = m_bUTF8 ? orm::utils::ReplaceQuotes(m_strPageDescription).c_str() : html::utils::ConvertString(orm::utils::ReplaceQuotes(m_strPageDescription).c_str());

	static time_t tmLast = time(0);
	time_t tmCurr = time(0);

	if (tmCurr-tmLast < 30)
	{
		m_gLastProxySites.push_back(CSSPProxySiteInfo(tmCurr, strURI_utf8, strTitle_utf8, strDescription_utf8, m_strUserIP, m_strLanguage));
		return;
	}
	
	tmLast = tmCurr;

	UpdateUserSitesTable();
	return;

#if 0
	orm::CTable::ExecuteSQL("BEGIN TRANSACTION");

	//if (tmCurr-tmLast > 10)
	//{
		orm::CTable::ExecuteSQL("DELETE FROM LINKS WHERE time IS NOT NULL AND status='temp' AND time > 0 AND time+43200 < %i", (int)tmCurr);
		orm::CTable::ExecuteSQL("DELETE FROM LINKS WHERE status='delete'");
	//}

	//orm::CTable::CreateTable("UsersSites", "(url TEXT, title TEXT, description TEXT, UserIP TEXT, time INTEGER)");
	const orm::CTable oldTable =
		orm::CTable("UsersSites").Where("url='%s' AND UserIP='%s' AND time>%s", 
		strURI_utf8.c_str(), orm::utils::ReplaceQuotes(m_strUserIP).c_str(), to_string(tmCurr-24*3600).c_str()).GetAllTable();
	
	if (oldTable.GetRowsCount() != 0)
	{
		orm::CTable::ExecuteSQL("COMMIT TRANSACTION");
		return;
	}

	orm::CTable("UsersSites").Insert("'%s', '%s', '%s', '%s', %s, 0, '%s'",
		strURI_utf8.c_str(),
		strTitle_utf8.c_str(),
		strDescription_utf8.c_str(),
		orm::utils::ReplaceQuotes(m_strUserIP).c_str(),
		to_string(tmCurr).c_str(),
		m_strLanguage.c_str());

	orm::CTable::ExecuteSQL("COMMIT TRANSACTION");
#endif
}

curl_socket_t opensocket_callback(void *clientp,
                                   curlsocktype purpose,
                                   struct curl_sockaddr *addr)
{
	CSSPProxy *pThis = (CSSPProxy *)clientp;

	auto it = g_mapHostToIP.find(pThis->m_strHost);

	if (it != g_mapHostToIP.end() || (pThis->m_strHost.find(m_strCurrentDNS) != -1))
		return socket(addr->family, addr->socktype, addr->protocol);

	const string strIP = [](struct sockaddr SinAddr) -> string
	{
		char sz[1000];
		memset(sz, 0, 1000);
		sprintf(sz, "%i.%i.%i.%i", (unsigned char)SinAddr.sa_data[2], (unsigned char)SinAddr.sa_data[3], (unsigned char)SinAddr.sa_data[4], (unsigned char)SinAddr.sa_data[5]);
		return string(sz);
	}(addr->addr);

	if ((strIP == "0.0.0.0") || (strIP == "127.0.0.1"))
		return socket(addr->family, addr->socktype, addr->protocol);


	DEBUG_LOG("opensocket_callback addr=%s IP=%s", pThis->m_strHost.c_str(), strIP.c_str());

	g_mapHostToIP[pThis->m_strHost] = shared_ptr<CSSPProxy::CResolvedIP>(new CSSPProxy::CResolvedIP(strIP));

	return socket(addr->family, addr->socktype, addr->protocol);
}

void CSSPProxy::AddResolvedDNS(CURL *pCurl)
{
	return;
	const string strHost = m_strHost;

	m_strDelHost = (strHost.find(":") == -1) ? "-" + strHost + ":80" : "-" + strHost;

	if (m_hostDel)
		curl_slist_free_all(m_hostDel);

	m_hostDel = NULL;
	m_hostDel = curl_slist_append(NULL, m_strDelHost.c_str());
	curl_easy_setopt(pCurl, CURLOPT_RESOLVE, m_hostDel);

	curl_easy_setopt(pCurl, CURLOPT_OPENSOCKETDATA, this); 

	auto it = g_mapHostToIP.find(strHost);
	if (it == g_mapHostToIP.end())
	{
		curl_easy_setopt(pCurl, CURLOPT_OPENSOCKETFUNCTION, opensocket_callback);
		return;
	}

	if (time(0) - it->second->m_tmTime > 100)
	{
		g_mapHostToIP.erase(strHost);
		curl_easy_setopt(pCurl, CURLOPT_OPENSOCKETFUNCTION, opensocket_callback);
		return;
	}

	m_strOptHost = (strHost.find(":") == -1) ? 
		strHost+":80:"+it->second->m_strIP : 
		strHost+":"+it->second->m_strIP;

	if (m_host)
		curl_slist_free_all(m_host);

	m_host = NULL;
	m_host = curl_slist_append(NULL, m_strOptHost.c_str());

	DEBUG_LOG("Custom resolve host=%s", m_strOptHost.c_str());
	curl_easy_setopt(pCurl, CURLOPT_RESOLVE, m_host);
}

bool CSSPProxy::Continue(const string strHost0, const string strURI, const map<string, string> &mapHeaders, vector<BYTE> *pInBuffer, vector<BYTE> *pOutBuffer, const bool bIsSSL, const string strIP, const map<string, string> &mapValues)
{
	const string strUserAgent = [mapValues]()->string
		{
			if (mapValues.find(USER_AGENT_3S3S) == mapValues.end())
				return "";
			return curl::CUrl::URLDecode(mapValues.at(USER_AGENT_3S3S));
		}();

	m_strCurrentDNS = DNS_NAME;
	m_strProxy = [mapValues]()->string
		{
			if (mapValues.find(PROXY_3S3S) == mapValues.end())
				return "";
			return curl::CUrl::URLDecode(mapValues.at(PROXY_3S3S));
		}();

	if (!strUserAgent.length())
		m_strUserAgent = "";//stagefright/1.2 (Linux;Android 5.0)";
	else
		m_strUserAgent = strUserAgent;

	//if (!m_bIsStarted)
	//{
		vector<string> vBlackListHosts;
		/*vBlackListHosts.push_back("utarget.ru");
		vBlackListHosts.push_back("adriver.ru");
		vBlackListHosts.push_back(".turn.com");
		vBlackListHosts.push_back("cdn.w55c.net");
		vBlackListHosts.push_back("facetz.net");
		vBlackListHosts.push_back("t.adonly.com");
		vBlackListHosts.push_back("adru.net");
		vBlackListHosts.push_back(".advertising.com");
		vBlackListHosts.push_back(".adnxs.com");
		vBlackListHosts.push_back(".contextweb.com");
		vBlackListHosts.push_back(".doubleclick.net");*/
		
		for (size_t nI = 0; nI<vBlackListHosts.size(); nI++)
		{
			if (strHost0.find(vBlackListHosts[nI]) != -1)
			{
				m_strReadedHeader = 
					"HTTP/1.1 200 OK\r\n"
					"Content-Type: text/plain; charset=utf-8\r\n"
					"Content-Length: 0\r\n"
					"\r\n";

				pOutBuffer->clear();
				UpdateCache(m_strReadedHeader.c_str(), m_strReadedHeader.length(), pOutBuffer, true);
				m_strReadedHeader = "";

				OnAllProxyDone();
				return true;
			}
		}
	//}


	if (!m_strLanguage.length())
	{
		m_strLanguage = [mapHeaders]() -> string
			{
				if (mapHeaders.find("Accept-Language") != mapHeaders.end())
					return mapHeaders.at("Accept-Language");
				if (mapHeaders.find("accept-language") != mapHeaders.end())
					return mapHeaders.at("accept-language");
				return "en";
			}();
	}

	m_strUserIP = strIP;

//	if (strHost0 == "ads3")
//		g_vWhiteListedIP.insert(m_strUserIP);
#ifndef _DEBUG
	if (m_bIsStarted && time(NULL)-m_tmLastTime > 60)
		return false;
#endif
#ifdef _DEBUG
	DEBUG_LOG("CSSPProxy::Continue ip=%s; uri=%s", m_strUserIP.c_str(), strURI.c_str());
#endif

	//DEBUG_LOG("CSSPProxy::Continue start");
	bool bProxyMode2 = false;
	const string strHost = [strHost0, bIsSSL](bool *pProxyMode2) -> string
		{
			const string strRetHost = (strHost0.find("h_t_t_p_s.") == 0) ? strHost0.substr(10) : strHost0;

			/*for (size_t n=0; n<m_vRegisteredDNS.size(); n++)
			{
				if (m_vRegisteredDNS[n].DNS() == strRetHost)
				{
					*pProxyMode2 = true;
					if (bIsSSL) 
						return m_vRegisteredDNS[n].IP() + ":" + to_string((int64_t)m_vRegisteredDNS[n].SSLPort());
					
					return m_vRegisteredDNS[n].IP() + ":" + to_string((int64_t)m_vRegisteredDNS[n].Port());
				}
			}*/
			return strRetHost;

		}(&bProxyMode2);

	//DEBUG_LOG("CSSPProxy::Continue strHost=%s", strHost.c_str());
//	string str = ChangeReferer("https://m.facebook.com");//\r\nAccept_Encoding: deflate\r\nAccept:text/html\r\n\r\n");
	string strLocation = GetLocation(strHost);
	if (strLocation.length())
	{
		DEBUG_LOG("strLocation=%s", strLocation.c_str());
		m_bIsLocation = true;
		DEBUG_LOG("Proxy Location: %s", strLocation.c_str());
		/*if ((strHost.length() >= 2) && strHost.rfind(".0") != strHost.length()-2)
		{
			if ((strHost.rfind('_') == strHost.length()-1) || 
				(strHost.rfind('0') == strHost.length()-1))
			{
				gCURL.DeleteHandle(m_pCurl);
				m_pCurl = NULL;
				m_bDone = true;
				return Redirect(pOutBuffer, strLocation);			
			}
		}
*/
	}

	//DEBUG_LOG("strLocation=%s (strURI=%s)", strLocation.c_str(), strURI.c_str());
	if (strLocation.length() && strURI.find("/") == 0 && strURI.length() > 1)
	{
		const string strFirst = strLocation.substr(0, strLocation.find("://"));
		const string strRest = strLocation.substr(strLocation.find("://")+3);
		if (strRest.find("/") != -1)
		{
			const string strLocHost = strRest.substr(0, strRest.find("/"));
			strLocation = strFirst + "://" + strLocHost;

			DEBUG_LOG("Change strLocation to %s (strURI=%s)", strLocation.c_str(), strURI.c_str());
		}
	}

	if (m_bIsStarted && !gCURL.HaveHandle(m_pCurl))
	{
		DEBUG_LOG("CSSPProxy::Continue return false1");
		return false;
	}

	vector<BYTE> *pReadedBytes = gCURL.GetReadedBytes(m_pCurl);

	if (gCURL.IsDone(m_pCurl))
	{
		DEBUG_LOG("CURL DONE! host=%s", strHost.c_str());
		if (m_strReadedHeader.length())
			return FlushHeader(pOutBuffer);

		if (pReadedBytes->size())
			return FlushBody(pOutBuffer);

		pOutBuffer->clear();
		if (m_bChunked)
		{
			//pOutBuffer->resize(5);
			//memcpy(&((*pOutBuffer)[0]), "0\r\n\r\n", 5);
			UpdateCache("0\r\n\r\n", 5, pOutBuffer, true);
		}
		else
			UpdateCache(0, 0, pOutBuffer, true);

		OnAllProxyDone();
		return true;
	}

	const string strMethod = (mapHeaders.find("Method") == mapHeaders.end()) ? "GET" : mapHeaders.at("Method");
	m_bInProxyMode = [strMethod, mapHeaders]() -> bool 
		{
			if (mapHeaders.find(strMethod) == mapHeaders.end())
				return false;
			if (mapHeaders.at(strMethod).find("http://") != 0)
				return false;
			return true;
		} ();

	m_bInProxyMode2 = bProxyMode2 ? true : [strHost, mapHeaders]() -> bool 
		{
			if (mapHeaders.find("Host") == mapHeaders.end())
			{
				DEBUG_LOG("m_bInProxyMode2 = true; (Host header not found) strHost=%s", strHost.c_str());
				return true;
			}
			if (mapHeaders.at("Host") == strHost)
			{
				if ((mapHeaders.find("Referer") != mapHeaders.end()) && (mapHeaders.at("Referer").find("3s3s.github.io") == -1))
				{
					DEBUG_LOG("m_bInProxyMode2 = true; strHost=%s; mapHeaders[Referer] = %s", strHost.c_str(), mapHeaders.at("Referer").c_str());
					return true;
				}
			}
			return false;
		} ();

#if 1
	if (strURI == "/robots.txt" && !strLocation.length() && !m_bInProxyMode && !m_bInProxyMode2)//.find("/robots.txt") == strURI.length()-1)
	{
		const string strRobots = 
			"User-agent: *\r\n"
			"Disallow: /";

		m_strReadedHeader = 
			"HTTP/1.1 200 OK\r\n"
			"Content-Type: text/plain; charset=utf-8\r\n"
			"Content-Length: " + to_string((long long)strRobots.length()) + "\r\n"
			"\r\n" + strRobots;

		pOutBuffer->clear();
		UpdateCache(m_strReadedHeader.c_str(), m_strReadedHeader.length(), pOutBuffer, true);
		m_strReadedHeader = "";

		OnAllProxyDone();
		return true;
	}
#endif
	//DEBUG_LOG("strHost=%s; m_bInProxyMode=%i; m_bInProxyMode2=%i", strHost.c_str(), m_bInProxyMode, m_bInProxyMode2);

	if ((strHost0 == "ads3" && !m_bInProxyMode) || (strHost0 == "cur.lv"))
	{
		DEBUG_LOG("Add to White List ip=%s", m_strUserIP.c_str());
		g_vWhiteListedIP.insert(m_strUserIP);
	}
	else
	{
		//DEBUG_LOG("No added to white  ip=%s; uri=%s; host=%s", m_strUserIP.c_str(), strURI.c_str(), strHost0.c_str());
	}

	if (!m_bIsStarted)
	{
		DEBUG_LOG("CSSPProxy::Continue (!m_bIsStarted) try to start");

		//if (bIsSSL && strHost0.find("h_t_t_p_s.") != 0)
		//	return MoveSSL(pOutBuffer, strLocation);

		m_bIsStarted = true;
		m_tmLastTime = time(NULL);
		

		if (m_pCurl)
			gCURL.DeleteHandle(m_pCurl);
		
		const string strFullURI = [this, strURI]() -> string
			{
				string strRet = strURI;

				if (!strRet.length() || (strRet.find("/") != 0))
					return "/" + strURI;
				return strRet;
			}();
		DEBUG_LOG("strHost0='%s', strFullURI='%s', strLocation='%s'", strHost0.c_str(), strFullURI.c_str(), strLocation.c_str());

		m_strURL = [=](string strHost) -> string
			{
				string strRetHost = strHost;
				/*for (size_t n=0; n<m_vRegisteredDNS.size(); n++)
				{
					if (m_vRegisteredDNS[n].DNS() == strRetHost)
					{
						if (bIsSSL) 
							strRetHost = m_vRegisteredDNS[n].IP() + ":" + to_string((int64_t)m_vRegisteredDNS[n].SSLPort());
						else
							strRetHost = m_vRegisteredDNS[n].IP() + ":" + to_string((int64_t)m_vRegisteredDNS[n].Port());
						break;
					}
				}*/

				return (bIsSSL ? "https://" : "http://") + strRetHost + strFullURI;
			}(strHost0);

		if (strLocation.length())
		{
			m_strURL = strLocation;
			if (strFullURI.length() > 1)
			{
				if (strLocation.rfind('/') != strLocation.length()-1)
					m_strURL += strFullURI;
				else
				{
					m_strURL += strFullURI.substr(1);
				}
			}
		}
		DEBUG_LOG("m_strURL='%s'", m_strURL.c_str());

		m_strURL = [](const string strURL) -> string
			{
				if (strURL.find("http://h_t_t_p_s.") != 0)
					return strURL;
				//if (strURL.find("youtube.com") != -1)
				//	return "http://"+strURL.substr(17);

				return "https://"+strURL.substr(17);
			}(m_strURL);
		
		m_strURL = [this](const string strURL) -> string
			{
				string strRet = strURL;
				int nPos = strRet.find(string(".") + m_strCurrentDNS);
				while ((nPos = strRet.find(string(".") + m_strCurrentDNS)) != strRet.npos)
				{
					string strLeft = strRet.substr(0, nPos);
					string strRight = strRet.substr(nPos + m_strCurrentDNS.length() + 1);
					strRet = strLeft+strRight;
				}
				return strRet;
			}(m_strURL);

		m_strHost = strHost;

		DEBUG_LOG("try curl %s", m_strURL.c_str());
		m_pCurl = curl_easy_init();
		curl_easy_setopt(m_pCurl, CURLOPT_URL, m_strURL.c_str());

		AddResolvedDNS(m_pCurl);
		
		curl_easy_setopt (m_pCurl, CURLOPT_FOLLOWLOCATION, 0);

		if (m_strProxy.length())
		{
			curl_easy_setopt (m_pCurl, CURLOPT_PROXY, m_strProxy.c_str());
		}

		curl_easy_setopt (m_pCurl, CURLOPT_MAXREDIRS, 10);
		curl_easy_setopt(m_pCurl, CURLOPT_HEADERFUNCTION, CURL_Header);
		curl_easy_setopt(m_pCurl, CURLOPT_HEADERDATA, this);
		curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYPEER, 0);
		curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYHOST, 0);

		curl_easy_setopt(m_pCurl, CURLOPT_USERAGENT, m_strUserAgent.c_str());

		curl_easy_setopt(m_pCurl, CURLOPT_DEBUGFUNCTION, curl::CUrl::Log);
		curl_easy_setopt(m_pCurl, CURLOPT_VERBOSE, 1L);
		curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, curl::CUrl::DownloadCallback);
		curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, m_pCurl);

		gCURL.AddHandle(m_pCurl);
		
		if (strMethod == "POST")
		{
			gCURL.SetOptPost(m_pCurl, pInBuffer);

			//m_vStartInPostBuffer = *pInBuffer;
			//curl_easy_setopt(m_pCurl, CURLOPT_POSTFIELDS, &m_vStartInPostBuffer[0]);
			//curl_easy_setopt(m_pCurl, CURLOPT_POSTFIELDSIZE, m_vStartInPostBuffer.size());
			curl_easy_setopt(m_pCurl, CURLOPT_READFUNCTION, curl::CUrl::ReadCallback);
			curl_easy_setopt(m_pCurl, CURLOPT_READDATA, m_pCurl);
			//m_vClientHeaders.push_back("Transfer-Encoding: chunked");
			//CURLcode res = curl_easy_perform(m_pCurl);
			//return true;
		}

		/*if (strMethod == "POST")
		{
			gCURL.SetOptPost(m_pCurl, pInBuffer);
			curl_easy_setopt(m_pCurl, CURLOPT_READFUNCTION, curl::CUrl::ReadCallback);
			curl_easy_setopt(m_pCurl, CURLOPT_READDATA, m_pCurl);
			curl_easy_setopt(m_pCurl, CURLOPT_POSTFIELDSIZE, 50);

			//m_vClientHeaders.push_back("Transfer-Encoding: chunked");
			//curl_easy_setopt(m_pCurl, CURLOPT_POST, true);
			//curl_easy_setopt(m_pCurl, CURLOPT_POSTFIELDS, "{ action: 'enter', login: '123', password: '123' }");
		}*/
		
		DEBUG_LOG("SetClientHeaders");
		SetClientHeaders(mapHeaders, strHost, strLocation);
		DEBUG_LOG("gCURL.AddHeaders");

#if 0
		m_vClientHeaders.clear();
		m_vClientHeaders.push_back("Host: docs.google.com");
		m_vClientHeaders.push_back("Mozilla/5.0 (Windows NT 6.3; WOW64; rv:34.0) Gecko/20100101 Firefox/34.0");
		m_vClientHeaders.push_back("Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
		m_vClientHeaders.push_back("Accept-Language: en-US");
		m_vClientHeaders.push_back("Connection: keep-alive");
		m_vClientHeaders.push_back("Accept-Encoding: no");
#endif
		gCURL.AddHeaders(m_pCurl, m_vClientHeaders);
		
		DEBUG_LOG("CURL added handle");
		return true;
	}

	gCURL.Pause(m_pCurl, false);

#ifdef _DEBUG
	DEBUG_LOG("pReadedBytes->size()=%i; m_strReadedHeader.length()=%i", pReadedBytes->size(), m_strReadedHeader.length());
#endif
	//if (m_strReadedHeader.length() && pReadedBytes->size()) //commented for correct work with POST
	//if (m_strReadedHeader.length() && (strMethod == "POST" || pReadedBytes->size()))
	if ((m_strReadedHeader.find("\r\n\r\n") != -1 || m_strReadedHeader.find("\n\n") != -1) && 
		(strMethod == "POST" || pReadedBytes->size()))
		return FlushHeader(pOutBuffer);
	
	if (pReadedBytes->size())
	{
		//if (!m_bFirstBodySended && m_bTextHtml && pReadedBytes->size() < 10000)
		/*if ((m_bTextCSS || m_bTextHtml) && pReadedBytes->size() < 16000)
		{
			//DEBUG_LOG("CSSPProxy::Continue pReadedBytes->size() < 16000 return true");
			return true;
		}*/

		if (m_bTextHtml && pReadedBytes->size() < 10000) {}
		else
			FlushBody(pOutBuffer);
	}

	gCURL.AddWritedBytes(m_pCurl, pInBuffer);
	return true;
}

const string CSSPProxy::AddContentLength(const string &strHeader, const int nInjectedLength)
{
	string strCurrLen = strHeader.substr(strHeader.find("Content-Length: ")+16);
	strCurrLen.erase (std::remove(strCurrLen.begin(), strCurrLen.end(), ' '), strCurrLen.end());
	strCurrLen.erase (std::remove(strCurrLen.begin(), strCurrLen.end(), '\r'), strCurrLen.end());
	strCurrLen.erase (std::remove(strCurrLen.begin(), strCurrLen.end(), '\n'), strCurrLen.end());

	ostringstream temp;
	temp << "Content-Length: " << (atoi(strCurrLen.c_str())+nInjectedLength) << "\r\n";
	string strRet = temp.str();
	return strRet;
}

size_t CSSPProxy::CURL_Header( void *ptr, size_t size, size_t nmemb, void *userdata)
{
	//DEBUG_LOG("CSSPProxy::CURL_Header start");
	if ((size*nmemb) && (userdata))
	{
		CSSPProxy *pThis = (CSSPProxy *)userdata;
		if (pThis->m_strReadedHeader.find("\r\n\r\n") != -1)
		{
			pThis->m_strReadedHeader = "";
			pThis->m_bTextHtml = false;
			pThis->m_bGZIP = false;
			pThis->m_bTextCSS = false;
			pThis->m_bApplicationJS = false;
		}
		DNS_NAME = pThis->m_strCurrentDNS;

		vector<unsigned char> temp((size+1)*nmemb);
		memcpy(&temp[0], ptr, size*nmemb);
		temp[size*nmemb] = 0;
		
		string strTemp = (const char*)&temp[0];// = data;

		if (strTemp.find("HTTP/1.0") != -1)
			strTemp.replace(strTemp.find("HTTP/1.0"), sizeof("HTTP/1.1")-1, "HTTP/1.1");//"HTTP/1.0", "HTTP/1.1");

		if (strTemp.find("HTTP/1.") != -1)
			pThis->m_strResponceCode = strTemp;

			//pThis->m_bTextHtml = true;
		if (strTemp.find("Content-Type: ") != -1 || strTemp.find("content-type: ") != -1)
		{
			pThis->m_bHaveContentType = true;
			string strCT = ChangeContentType(pThis->m_strURL, strTemp);
			strTemp = strCT;

			if (strTemp.find("text/html") != -1)
			{
				DEBUG_LOG("pThis->m_bTextHtml = true; (Content-Type: text/html) strTemp=%s", strTemp.c_str());
				pThis->m_bTextHtml = true;
			}
			if ((strTemp.find("utf-8") != -1) || (strTemp.find("UTF-8") != -1))
				pThis->m_bUTF8 = true;
		}
		if (strTemp.find("Content-Type: ") != -1 || strTemp.find("content-type: ") != -1)
		{
			if (strTemp.find("text/css") != -1)
				pThis->m_bTextCSS = true;
			if (strTemp.find("javascript") != -1)
				pThis->m_bApplicationJS = true;
		}

		if (strTemp.find("gzip") != -1)
			pThis->m_bGZIP = true;
		//if (strTemp.find("Connection") != -1)
		//	strTemp = "Connection: close\r\n";
		//if ((strTemp.find("Content-Length:") != -1) && (pThis->m_bTextHtml))
		//	strTemp = AddContentLength(strTemp, pThis->m_strInjectString.length());
//		if ((strTemp.find("Content-Length:") != -1) && (pThis->m_bTextHtml || pThis->m_bTextCSS))
	//		strTemp = "Transfer-Encoding: chunked\r\n";
		if (!pThis->m_bInProxyMode && !pThis->m_bInProxyMode2)
		{
			if (strTemp.find("Location:") != -1 || strTemp.find("location:") != -1)
			{
				const string strProto = (strTemp.find("http://") == -1)?"https://":"http://";
				strTemp = ChangeLocation(strTemp, strProto);

				if (strTemp.find("ocation: https://") == 1)
				{
					string str = "Location: http://h_t_t_p_s." + strTemp.substr(18);
					strTemp = str;
				}
			}
		/*}

		if (!pThis->m_bInProxyMode)
		{*/
			const int n1 = strTemp.find("Set-Cookie:");
			const int n2 = strTemp.find("set-cookie:");
			if (n1 != -1 || n2 != -1)
			{
				strTemp = ChangeCookie(strTemp, pThis->m_strURL.find("https://") == 0);
				/*if (pThis->m_strCookies.length())
				{
					int nEnd = pThis->m_strCookies.rfind("\r\n");
					if (nEnd == -1)
						nEnd = pThis->m_strCookies.rfind("\n");
					if (nEnd == -1)
						pThis->m_strCookies.length()-1;

					const int nPos = (n1==-1)?n2:n1;
					string str = pThis->m_strCookies.substr(0, nEnd)+"\n" + strTemp.substr(nPos+11);
					strTemp = str;
				}
				pThis->m_strCookies = strTemp;*/
			}

		}

		//if (!pThis->m_bInProxyMode && !pThis->m_bInProxyMode2)
		//{
			if (strTemp.find("Content-MD5:") != -1 || strTemp.find("Cache-Control:") != -1 || 
				strTemp.find("X-Frame-Options:") != -1 || strTemp.find("x-frame-options:") != -1 ||
				strTemp.find("Timing-Allow-Origin:") != -1 || strTemp.find("timing-allow-origin:") != -1 ||
				strTemp.find("Transfer-Encoding:") != -1 || strTemp.find("Connection:") != -1 ||
				strTemp.find("content-security-policy:") != -1 || strTemp.find("Content-Security-Policy:") != -1 ||
				strTemp.find("X-Content-Type-Options:") != -1 ||
				strTemp.find("Access-Control-Allow-Origin:") != -1 || strTemp.find("access-control-allow-origin:") != -1)
				strTemp = "";
		//}
		//if (strTemp.find("X_Cache") != -1)
			//	strTemp = DeleteSuffix(strTemp);
		
		if (strTemp.find("Content-Length:") != -1 || strTemp.find("content-length:") != -1)
			pThis->m_strContentLength = strTemp;
		else
		{
			pThis->m_strReadedHeader += strTemp;
		}

		if (!pThis->m_bInProxyMode && !pThis->m_bInProxyMode2)
		{
			if (pThis->m_strReadedHeader.find("Access-Control-Allow-Origin:") == -1)
				pThis->m_strReadedHeader += "Access-Control-Allow-Origin: *\r\n";
		}
		//if ((strTemp.find("Access-Control-Allow-Origin:") != -1) || (strTemp.find("access-control-allow-origin:") != -1))
		//	strTemp = "";//"Access-Control-Allow-Origin: *\r\n";
	}
	//DEBUG_LOG("CSSPProxy::CURL_Header end");
	return size*nmemb;
}