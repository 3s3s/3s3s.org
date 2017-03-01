#ifndef _startupinfo
#define _startupinfo
#include "log.h"
#include <unordered_map>


#include "simple_server.h"

using namespace curl;
extern CUrl gCURL;

namespace startup
{
	enum ACTION
	{
		A_START,
		A_CONTINUE,
		A_END
	};

	class CRegisteredDNS
	{ 
		string m_strDNS, m_strIP;
		int m_nPort, m_nSSLPort;
	public:
		CRegisteredDNS(const string strDNS, const string strIP, int nPort, int nSSLPort) : 
		  m_strDNS(strDNS), m_strIP(strIP), m_nPort(nPort), m_nSSLPort(nSSLPort) {}
		const string DNS() const {return m_strDNS;}
		const string IP() const {return m_strIP;}
		const int Port() const {return m_nPort;}
		const int SSLPort() const {return m_nSSLPort;}
	};

	class CPostContent
	{
		map<string, vector<string> > m_mapHeader;
		vector<BYTE> m_vBody;
	public:
		CPostContent(string strHeader, vector<BYTE> vBuffer) :
		  m_vBody(vBuffer)
		{
			if (strHeader.length())
				ParseHeader(strHeader);
		}

	    vector<BYTE> GetBody()
		{
			return m_vBody;
		}
		static size_t ParseLine(int nPos, string &strLines, string &strFirst, string &strSecond)
		{
			size_t nRet = strLines.length();
			string substr = strLines.substr(nPos);
			if (substr.length() < 4)
				return nRet;

			int nEnd = substr.find("\r\n");
			if ((nEnd == substr.npos) || (nEnd == 0))
				return nRet;

			nRet = nPos + nEnd+2;

			string strLine = substr.substr(0, nEnd);
			
			int nSpace = strLine.find(" ");
			if (nPos > 0)
				nSpace = strLine.find(": ");

			if (nSpace != strLine.npos)
				strFirst = strLine.substr(0, nSpace);
			else 
				nSpace = -1;
			
			string substr2 = strLine.substr(nSpace+1);
			if (nPos != 0)
			{
				strSecond = substr2;
				return nRet;
			}
			nSpace = substr2.find(" ");
			if (nSpace == substr2.npos)
				strSecond = substr2;
			else
				strSecond = substr2.substr(0, nSpace);

			return nRet;
		}
		void AddBuffer(vector<BYTE> vBuffer)
		{
			m_vBody.insert(m_vBody.end(), vBuffer.begin(), vBuffer.end());
		}
		void EraseBody()
		{
			m_vBody.clear();
		}
		vector<string> ParseSecond(string strSecond)
		{
			vector<string> ret;

			//strSecond.erase(remove_if(strSecond.begin(), strSecond.end(), isspace), strSecond.end());
			string str = strSecond;
			
			int nPos = 0;
			string strNext = str;
			while (nPos != str.npos)
			{
				str = strNext;
				nPos = str.find(";");

				if (nPos != str.npos)
					ret.push_back(str.substr(1, nPos-1));
				else
				{
					ret.push_back(str.substr(1));
					break;
				}
				
				strNext = str.substr(nPos+1);
			}
			return ret;
		}
		string GetName()
		{
			string strRet;
			if (m_mapHeader.find("Content-Disposition") == m_mapHeader.end())
				return strRet;

			vector<string> values = m_mapHeader["Content-Disposition"];
			for (size_t n=0; n<values.size(); n++)
			{
				int nPos = values[n].find("name=");
				if (nPos == values[n].npos)
					continue;
				return values[n].substr(nPos+5);
			}
			return strRet;
		}
		void ParseHeader(string strHeader)
		{
			size_t nPos = strHeader.find("\r\n")+2;
			while (1)
			{
				string strFirst, strSecond;
				nPos = ParseLine(nPos, strHeader, strFirst, strSecond);

				if (nPos == strHeader.length())
					break;

				vector<string> vSecond = ParseSecond(strSecond);
				m_mapHeader[strFirst] = vSecond;
			}
		}
	};

	class CClient
	{
	public:
		virtual bool ContinueGet(vector<BYTE> *pOutBuffer) = 0;

		const bool IsSSL() const {return m_bIsSSL;}

	protected:
		bool m_bAllRecieved;
		unsigned long long m_llContentLength, m_llCurrentBodyLength;

		const bool IsAllRecieved() const {return m_bAllRecieved;}
		const string GetIP() const {return m_strIP;}

		const map<string, string> &GetMapNameToValue() const {return m_mapNameToValue;}
		const string GetBrowserLang()
		{
			string strVal;
			if (m_mapNameToValue.find("Accept-Language") != m_mapNameToValue.end())
				strVal = m_mapNameToValue["Accept-Language"];
			if (m_mapNameToValue.find("accept-language") != m_mapNameToValue.end())
				strVal = m_mapNameToValue["accept-language"];

			if (strVal.find("ru") != strVal.npos)
				return "ru";
			if (strVal.find("en") != strVal.npos)
				return "en";
			if (strVal.find("jp") != strVal.npos)
				return "jp";
			
			return "";
		}

	private:
		int m_nID;
		ACTION m_nAction;
		bool m_bNeadDelete, m_bGzip;
		vector<string> m_vQueryes;
		string m_strMethod, m_strURI, m_strContentType;
		string m_strBoundary, m_strIP, m_strQuery;

		map<string, string> m_mapNameToValue;
		bool m_bIsSSL;


		vector<CPostContent> m_PostContent;
		vector<BYTE> m_vUnparsedPost;

		void OnAllRecieved() 
		{
			m_bAllRecieved = true;
			if ((m_strContentType.find("application/x-www-form-urlencoded") != m_strContentType.npos) && (!m_vQueryes.size()))
			{
				vector<BYTE> vBody(GetPostBody("", true));
				vBody.push_back(0);
				m_vQueryes = simple_server::CHttpHeader::ParseQuery((const char *)&(vBody[0]));
			}
		}
		string GetHeaderAndTail(vector<BYTE> *pBody)
		{
			string strRet;
			pBody->clear();

			if (m_vUnparsedPost.size() < m_strBoundary.length())
				return strRet;

			//Ищем границу в поступивших данных
			vector<BYTE>::iterator itStart = m_vUnparsedPost.end(), itEnd;

			if (m_strBoundary.length())
				itStart = search (
					m_vUnparsedPost.begin(), m_vUnparsedPost.end(), 
					m_strBoundary.c_str(), m_strBoundary.c_str()+m_strBoundary.length());

			//int nPos = itStart-m_vUnparsedPost.begin();
			if (itStart == m_vUnparsedPost.end())
			{
				//граница не найдена
				//Возвращаем часть хвоста, как тело
				vector<BYTE> vBody(m_vUnparsedPost.begin(), m_vUnparsedPost.end()-m_strBoundary.length());
				if (!vBody.size())
					return strRet;

				vector<BYTE> vTail(m_vUnparsedPost.end()-m_strBoundary.length(), m_vUnparsedPost.end());

				*pBody = vBody;
				m_vUnparsedPost = vTail;
				return strRet;
			}

			//Граница найдена, смотрим - есть ли над ней данные
			vector<BYTE> vPrevBody(m_vUnparsedPost.begin(), itStart);
			if (vPrevBody.size())
			{
				//Найдены данные над грницей - возвращаем их без заголовка
				//потому что они относятся к предыдущему заголовку
				vector<BYTE> vTail(itStart, m_vUnparsedPost.end());
				*pBody = vPrevBody;
				m_vUnparsedPost = vTail;
				return strRet;
			}

			//ищем конец заголовка
			const char *pszEnd = "\r\n\r\n";
			itEnd = search (
				itStart, m_vUnparsedPost.end(), 
				pszEnd, pszEnd + 4);

			
			if (itEnd == m_vUnparsedPost.end())
			{
				//Конец заголовка не найден, наверное он еще не весь пришел.
				//Но проверим, может быть уже пришел конец всех данных
				string strEnd = m_strBoundary + "--";
				itEnd = search (
					itStart, m_vUnparsedPost.end(), 
					strEnd.c_str(), strEnd.c_str()+strEnd.length());

				if (itEnd != m_vUnparsedPost.end())
				{
					//Да все данные приняты
					OnAllRecieved();
				}

				return strRet;
			}

			//Конец заголовка найден записываем его и хвост в разные массивы
			vector<BYTE> vHeader(itStart, itEnd+4);
			vector<BYTE> vTail(itEnd+4, m_vUnparsedPost.end());

			m_vUnparsedPost = vTail;
			vHeader.push_back(0);
			strRet = (const char *)(&vHeader[0]);

			//Ищем в хвосте следующую границу
			if (m_vUnparsedPost.size() < m_strBoundary.length())
				return strRet; //слишком короткий хвост, возвращаем заголовок без тела

			itStart = search (
				m_vUnparsedPost.begin(), m_vUnparsedPost.end(), 
				m_strBoundary.c_str(), m_strBoundary.c_str()+m_strBoundary.length());

			if (itStart == m_vUnparsedPost.end())
			{
				//следующая граница не найдена
				//Возвращаем часть хвоста, как тело
				vector<BYTE> vBody(m_vUnparsedPost.begin(), m_vUnparsedPost.end()-m_strBoundary.length());
				vector<BYTE> vLastTail(m_vUnparsedPost.end()-m_strBoundary.length(), m_vUnparsedPost.end());

				*pBody = vBody;
				m_vUnparsedPost = vLastTail;
				return strRet;
			}

			vector<BYTE> vBody(m_vUnparsedPost.begin(), itStart);
			vector<BYTE> vNextTail(itStart, m_vUnparsedPost.end());
			
			*pBody = vBody;
			m_vUnparsedPost = vNextTail;

			return strRet;
		}
		void ParsePost()
		{
			if (m_bAllRecieved)
				return;
			
			string strHeader;
			vector<BYTE> vBody;
			
			while(!m_bAllRecieved)
			{
				strHeader = GetHeaderAndTail(&vBody);

				m_llCurrentBodyLength += vBody.size();

				if (vBody.size() || strHeader.length())
				{
					if (!strHeader.length())
					{
						//Если пришло тело без заголовка, значит это тело 
						//относися к предыдущему заголовку
						if (m_PostContent.size())
							m_PostContent[m_PostContent.size()-1].AddBuffer(vBody);
						else
						{
							//ну или заголовка не предусмотрено
							CPostContent content(strHeader, vBody);
							m_PostContent.push_back(content);
							break;
						}
					}
					else
					{
						CPostContent content(strHeader, vBody);
						m_PostContent.push_back(content);
					}
				}
				
				if (!vBody.size())
					break;									
			}

			if (m_llCurrentBodyLength >= m_llContentLength)
				OnAllRecieved();
			//vBody.resize()

			//CPostContent content((const char*)(&vHeader[0]), itEnd+4);
			//m_PostContent.push_back(content);
		}
	public:

		//CVoiceFile m_VoiceFile;
		//CRegisterModule m_Register;

		CClient() {}
		~CClient()
		{
		}
		CClient(int nID, vector<string> &vQueryes, string strURI, unsigned long long llLength, const string strBoundary, const string strContentType,
			const string strIP, const string strQuery, const map<string, string> &mapNameToValue, const bool bIsSSL) :
			m_nID(nID), m_vQueryes(vQueryes), m_nAction(A_START), m_bNeadDelete(false), m_llContentLength(llLength), 
			m_bAllRecieved(false), m_strURI(strURI), m_strContentType(strContentType), m_llCurrentBodyLength(0),
			m_strIP(strIP), m_strQuery(strQuery), m_mapNameToValue(mapNameToValue), m_bIsSSL(bIsSSL)
		{
			string strEncode = ((mapNameToValue.find("Accept-Encoding") == mapNameToValue.end()) ? "" : mapNameToValue.at("Accept-Encoding"));
			m_bGzip = false;
			if (strEncode.find("gzip") != strEncode.npos)
				m_bGzip = true;
		
			m_strMethod = (mapNameToValue.find("Method") == mapNameToValue.end()) ? "" : mapNameToValue.at("Method");

			if (strBoundary.length())
				m_strBoundary = "--" + strBoundary;

		}

		const string GetHeaderValue(const string strKey) const 
		{
			if (m_mapNameToValue.find(strKey) == m_mapNameToValue.end())
				return "";
			return m_mapNameToValue.at(strKey);
		}
		const vector<string> GetAllHeaderValues() const
		{
			vector<string> vRet;
			for (auto it = m_mapNameToValue.begin(); it != m_mapNameToValue.end(); ++it)
				vRet.push_back(it->first + ": " + it->second + "\r\n");

			return vRet;
		}

		virtual bool ContinuePost(vector<BYTE> *pInBuffer, vector<BYTE> *pOutBuffer)
		{
			m_vUnparsedPost.insert(m_vUnparsedPost.end(), pInBuffer->begin(), pInBuffer->end());

			ParsePost();

			return true;
		}
		inline void Delete()
		{
			m_bNeadDelete = true;
		}
		inline void Continue()
		{
			if (m_bNeadDelete)
				return;

			switch(m_nAction)
			{
				case A_START:
					break;
				case A_CONTINUE:
					break;
				case A_END:
					break;
			}
		}
		const bool NeadDelete() const {return m_bNeadDelete;}
		const string GetMethod() const {return m_strMethod;}
		const string GetURI() const {return m_strURI;}
		const string GetQuery() const {return m_strQuery;}
		const string GetHost() const 
		{
			const string str = GetHeaderValue("Host");
			if (str.length()) return str;

			return GetHeaderValue("host");
		}
		const bool HasGzip() const {return m_bGzip;}
		const std::string GetContentType() const {return m_strContentType;}
		vector<BYTE> GetPostBody(string strName, bool bErase = false)
		{
			vector<BYTE> ret;
			for (size_t n=0; n<m_PostContent.size(); n++)
			{
				const string strContentName = m_PostContent[n].GetName();
				if (strContentName.length())
				{
					if (strContentName != "\""+strName+"\"")
						continue;
				}
				else
				{
					if (strContentName != strName)
						continue;
				}

				ret = m_PostContent[n].GetBody();
				if (bErase)
					m_PostContent[n].EraseBody();
				break;
			}
			return ret;
		}
		const string GetValue(const string &strKey) const
		{
			string strRet;
			if (NeadDelete())
				return strRet;

			for (size_t n=0; n<m_vQueryes.size(); n++)
			{
				size_t nPos = m_vQueryes[n].find("=");
				if (nPos == m_vQueryes[n].npos)
					nPos = m_vQueryes[n].find(":");
				
				if (nPos == m_vQueryes[n].npos)
					continue;

				if (strKey != m_vQueryes[n].substr(0, nPos))
					continue;

				strRet = m_vQueryes[n].substr(nPos+1);
				break;
			}
			return strRet;
		}
		const map<string, string> GetAllValues() const
		{
			map<string, string> ret;
			if (NeadDelete())
				return ret;

			for (size_t n=0; n<m_vQueryes.size(); n++)
			{
				size_t nPos = m_vQueryes[n].find("=");
				if (nPos == m_vQueryes[n].npos)
					nPos = m_vQueryes[n].find(":");

				if (nPos == -1)
					ret[m_vQueryes[n]] = "";
				else
					ret[m_vQueryes[n].substr(0, nPos)] = m_vQueryes[n].substr(nPos+1);
			}
			return ret;
		}
	};

template <class clientType>
class CStartupInfo// : public IStartupInfo
{
	unordered_map<int, clientType> m_Clients;
public:
	explicit CStartupInfo() {}
	CStartupInfo(const CStartupInfo& info) :
		m_nPort(info.m_nPort), m_nPortSSL(info.m_nPortSSL), m_strRoot(info.m_strRoot), m_strErrors(info.m_strErrors), m_strIndex(info.m_strIndex)
	{
	}
	CStartupInfo(int nPort, int nPortSSL, string strRoot, string strErrors, string strIndex) :
	  m_nPort(nPort), m_strRoot(strRoot), m_strErrors(strErrors), m_strIndex(strIndex), m_nPortSSL(nPortSSL)
	{
	}
	inline string GetRoot() {return m_strRoot;}
	inline string GetErrors() {return m_strErrors;}
	const string GetIndex() const {return m_strIndex;}
	inline int GetPort() {return m_nPort;}
	inline int GetPortSSL() {return m_nPortSSL;}
	inline void ContinuePlugins()
	{
		gCURL.Continue();
	
		size_t nFirstForDelete = -1;
		for (auto it = m_Clients.begin(); it != m_Clients.end(); ++it)
		{
			it->second.Continue();
			if ((nFirstForDelete == -1) && (it->second.NeadDelete()))
			{
				nFirstForDelete = it->first;
			}
		}
		if (nFirstForDelete != -1)
			m_Clients.erase(nFirstForDelete);
	}
	inline void RegisterCGIClient(
		int nClientID, vector<string> &vQueryes, string strURI, unsigned long long llLength, 
		const string &strBoundary, const string &strContentType, const string strIP, const string strQuery, const map<string, string> &mapNameToValue,
		const bool bIsSSL)
	{
		EraseClient(nClientID);
		m_Clients[nClientID] = clientType(nClientID, vQueryes, strURI, llLength, strBoundary, strContentType, strIP, strQuery, mapNameToValue, bIsSSL);
	}

	bool NeadCGI(const string &strURL, const string &strQuery, const map<string, string> &mapNameToValue) const
	{
		return clientType::NeadCGI(strURL, strQuery, mapNameToValue);
	}

	inline bool GetCGIInfo(int nClientID, vector<BYTE> *pInBuffer, vector<BYTE> *pOutBuffer)
	{
		//return thisPlugin.GetCGIInfo(nClientID, pInBuffer, pOutBuffer);
		pOutBuffer->clear();

		if ((m_Clients.find(nClientID) == m_Clients.end()) ||
			(m_Clients[nClientID].NeadDelete()))
		{
			DEBUG_LOG("GetCGIInfo return false (client not found or nead for delete) id=%i", nClientID);
			return false;
		}

		if (m_Clients[nClientID].GetMethod() == "POST")
			return m_Clients[nClientID].ContinuePost(pInBuffer, pOutBuffer);

		return m_Clients[nClientID].ContinueGet(pOutBuffer);
	}
	void EraseClient(int nClientID)
	{
		if (m_Clients.find(nClientID) == m_Clients.end())
			return;

		m_Clients.erase(nClientID);
	}
private:
	int m_nPort, m_nPortSSL;
	string m_strRoot, m_strErrors, m_strIndex;
};

}

#endif
