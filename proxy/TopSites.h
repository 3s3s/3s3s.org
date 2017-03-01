#ifndef _TOP_SITES_SSP
#define _TOP_SITES_SSP
#include "../StartupInfo.h"
#include "../utils/orm.h"
#include "Template.h"

using namespace std;
namespace proxy_site
{
	class CTopSitesSSP : public CTemplateSSP
	{
	private:
		const string GetTopList(const string strLang, const time_t tmDelta) const;
		const string GetTopListWeek(const string strLang) const;
		const string GetTopListDay(const string strLang) const;
		const string GetTopListHour(const string strLang) const;
		const orm::CTable FinalSortTable(const orm::CTable &tableSrc, const string strLang) const;
	protected:
		virtual const string LeftTopics(const string strLang) const;
		virtual const string MiddleContent(const string strLang) const;

		virtual const string GetTitle(const string strLang) const;
		virtual const string GetDescription(const string strLang) const;
		virtual const string GetKeywords(const string strLang) const;
	};
}


#endif