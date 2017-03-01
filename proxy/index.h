#ifndef _PROXY_INDEX_H
#define _PROXY_INDEX_H
#include "../StartupInfo.h"
#include "Template.h"

using namespace std;
namespace proxy_site
{
	class CIndexSSP : public CTemplateSSP
	{
		const string CreateShorUrlForm(const string strLang) const;
		const string UnblockSiteForm(const string strLang) const;

	protected:
		virtual const string LeftTopics(const string strLang) const;
		virtual const string MiddleContent(const string strLang) const;

		virtual const string GetTitle(const string strLang) const;
		virtual const string GetDescription(const string strLang) const;
		virtual const string GetKeywords(const string strLang) const;
	};
}
#endif