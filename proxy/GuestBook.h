#ifndef _Guest_Book_SSP
#define _Guest_Book_SSP
#include "../StartupInfo.h"
#include "../utils/orm.h"
#include "Template.h"

using namespace std;
namespace proxy_site
{
	class CGuestBookSSP : public CTemplateSSP
	{
	protected:
		virtual const string LeftTopics(const string strLang) const;
		virtual const string MiddleContent(const string strLang) const;

		virtual const string GetTitle(const string strLang) const;
		virtual const string GetDescription(const string strLang) const;
		virtual const string GetKeywords(const string strLang) const;
	};
}


#endif