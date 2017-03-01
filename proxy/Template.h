#ifndef _PROXY_TEMPLATE_SSP
#define _PROXY_TEMPLATE_SSP
#include "../StartupInfo.h"

using namespace std;
namespace proxy_site
{
	class CTemplateSSP
	{
		map<string, string> m_Values;
		string m_strIP;
	protected:
		const string GetValue(const string strKey) const 
		{
			if (m_Values.find(strKey) == m_Values.end())
				return "";
			return m_Values.at(strKey);
		}
		const map<string, string> GetValues() const {return m_Values;}
		const string GetIP() const {return m_strIP;}

		virtual const string ConvertString (const string strASCII) const;
		virtual const string LeftContent(const string strLang) const;
		virtual const string RightContent(const string strLang) const;
		virtual const string StartBody() const;
		
		virtual const string LeftTopics(const string strLang) const = 0;
		virtual const string MiddleContent(const string strLang) const = 0;
		virtual const string GetTitle(const string strLang) const = 0;
		virtual const string GetDescription(const string strLang) const = 0;
		virtual const string GetKeywords(const string strLang) const = 0;
		
	public:
		virtual void ContinueShow(vector<BYTE> *pOut, string strUTF8Content, bool bHasGZip) const;
		static const string ContentHeader(const string strLang);
	
		virtual void ShowEN(map<string, string> values, const string strIP, bool bHasGZip, vector<BYTE> *pOut);
		virtual void Show(map<string, string> values, const string strIP, bool bHasGZip, vector<BYTE> *pOut);
	};
}

#endif