#ifndef _MAKEURL
#define _MAKEURL
#include "../StartupInfo.h"
#include "../log.h"
#include "Template.h"

using namespace std;
namespace proxy_site
{
	class CMakeUrlSSP : public CTemplateSSP
	{
	protected:
		virtual const string LeftTopics(const string strLang) const {return "";};
		virtual const string MiddleContent(const string strLang) const {return "";};
		virtual const string GetTitle(const string strLang) const {return "";};
		virtual const string GetDescription(const string strLang) const {return "";};
		virtual const string GetKeywords(const string strLang) const {return "";};
	private:
		const string ValidateAlias(const string strAlias) const;
		const string ValidateLong(const string strLong) const;
		const string OnMakeNew(const string strLong, const string strAlias, const string &strIP, const string strStatus, time_t tmCurrent) const;
		const string GetNewShortLink() const;
		const string FillShortLinks(const string strLong, const string strStatus) const;
	public:
		virtual void Show(map<string, string> values, const string strIP, bool bHasGZip, vector<BYTE> *pOut);
	};
}

#endif