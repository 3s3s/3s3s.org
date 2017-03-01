#include "TopSites.h"
#include "../html_framework.h"
#include "Proxy.h"

using namespace proxy_site;
using namespace html;

const orm::CTable CTopSitesSSP::FinalSortTable(const orm::CTable &tableSrc, const string strLang) const
{
	orm::CTable ret("");
	for (int n=0; n<tableSrc.GetRowsCount(); n++)
	{
		if (tableSrc[n]["lang"].find(strLang) != -1)
			ret.AddRow(tableSrc[n]);
	}
	for (int n=0; n<tableSrc.GetRowsCount(); n++)
	{
		if (tableSrc[n]["lang"].find(strLang) == -1)
			ret.AddRow(tableSrc[n]);
	}
	return ret;
}

const string CTopSitesSSP::GetTopListWeek(const string strLang) const
{
	return GetTopList(strLang, 3600*24*7);
}
const string CTopSitesSSP::GetTopListDay(const string strLang) const
{
	return GetTopList(strLang, 3600*24);
}
const string CTopSitesSSP::GetTopListHour(const string strLang) const
{
	return GetTopList(strLang, 3600);
}

const string CTopSitesSSP::GetTopList(const string strLang, const time_t tmDelta) const
{
	DEBUG_LOG("GetTopList strLang=%s", strLang.c_str());
#ifndef _DEBUG
	const time_t tmCurrent = time(0);
#else
	const time_t tmCurrent = 24*3600;
#endif


	static time_t tmPrev = time(0);
	const time_t tmCurr = time(0);
#ifndef _DEBUG	
	if (tmCurr-tmPrev > 3600*24)
	{
		tmPrev = tmCurr;
		orm::CTable::ExecuteSQL("DELETE FROM UsersSites WHERE time<%i", tmCurr-10*3600*24*7);
		orm::CTable::ExecuteSQL("VACUUM");
	}
#endif

	//const orm::CTable table = orm::CTable("UsersSites").Where("time>%i  AND description <> '' GROUP BY url ORDER BY COUNT(url) DESC LIMIT 10", tmCurrent-24*3600).GetAllTableDistinct("*, COUNT(url) ");
#ifndef _DEBUG1
	const orm::CTable tab = orm::CTable("(SELECT * FROM(SELECT tmp.url, tmp.title, tmp.description, tmp.time, tmp.UserIP, tmp.flag, tmp.cURL, tmp.lang FROM (SELECT *, COUNT(url) AS cURL FROM UsersSites WHERE LENGTH(description) > 20 AND LENGTH(title) > 10 AND time>"+to_string(tmCurr-tmDelta)+" GROUP BY url ORDER BY cURL DESC LIMIT 2000) AS tmp GROUP BY title) GROUP BY description LIMIT 200)", " ORDER BY cURL DESC LIMIT 20").GetAllTableDistinct("*");
#else
	const orm::CTable tab = orm::CTable("(SELECT * FROM(SELECT tmp.url, tmp.title, tmp.description, tmp.time, tmp.UserIP, tmp.flag, tmp.cURL, tmp.lang FROM (SELECT *, COUNT(url) AS cURL FROM UsersSites WHERE LENGTH(description) > 20 AND LENGTH(title) > 10 GROUP BY url ORDER BY cURL DESC LIMIT 2000) AS tmp GROUP BY title) GROUP BY description ORDER BY time DESC LIMIT 200)", " ORDER BY cURL DESC, lang LIMIT 20").GetAllTableDistinct("*");
#endif

	const orm::CTable tableSorted = FinalSortTable(tab, strLang);

	string strRet;
	for (int n=0; n<tableSorted.GetRowsCount(); n++)
	{
		const string strTitle = [] (string strOriginal) -> string
		{
			utils::Replace(strOriginal, 0, "<", "&lt");
			utils::Replace(strOriginal, 0, ">", "&gt");

			while(strOriginal.find("  ") != -1) {strOriginal.replace(strOriginal.find("  "), 2, " ");}

			return strOriginal;

		}(tableSorted[n]["title"]);
		
		const string strDescription = [] (string strOriginal) -> string
		{
			utils::Replace(strOriginal, 0, "<", "&lt");
			utils::Replace(strOriginal, 0, ">", "&gt");

			while(strOriginal.find("  ") != -1) {strOriginal.replace(strOriginal.find("  "), 2, " ");}

			return strOriginal;

		}(tableSorted[n]["description"]);

		const string strProto = (tableSorted[n]["url"].find("http://") == -1)?"https://":"http://";

		const string strHREF = CSSPProxy::ChangeLocation(tableSorted[n]["url"], strProto);
		const string strLink = A("tl_link_title").AddAttribute(" target=\"_blank\"").href(strHREF).add(strTitle).outerHTML();
		strRet +=
				(li("top") << 
					(DIV() << h3().add(strLink) << 
						(DIV() << 
							DIV("tl_link").add(tableSorted[n]["url"]) << span().add(strDescription) <<
							(DIV() <<
								DIV("yashare-auto-init").AddAttribute
									(
										"data-yashareL10n=\"" + strLang + "\" "
										"data-yasharelink=\"" + strHREF + "\" "
										"data-yasharetitle=\"" + strTitle + "\" "
										"data-yasharedescription=\"" + strDescription + "\" "
										"data-yashareQuickServices=\"twitter,facebook,Lj,gplus,vkontakte,odnoklassniki,moimir\" "
										"data-yashareTheme=\"counter\""
									)

								/*A("twitter-share-button").AddAttribute(
									"data-lang=\"" + strLang + "\" "
									"data-text=\"" + strTitle + "\" "
									"data-url=\"" + strHREF + "\""
								).href("https://twitter.com/share").add("Tweet") <<
								DIV("fb-share-button").AddAttribute(
									"data-href=\""+strHREF+"\" "
									"data-layout=\"button_count\""
								)*/
							)
						))).outerHTML();
	}
	return strRet;
}

const string CTopSitesSSP::MiddleContent(const string strLang) const
{
	const string strListHour = GetTopListHour(strLang);
	const string strListDay = GetTopListDay(strLang);
	const string strListWeek = GetTopListWeek(strLang);

	const string strContentHour = (strLang == "en") ? 
		(DIV("middle_content") << h2("title_top").add("Top sites") << DIV("top_list").add(strListHour)).outerHTML() :
		(DIV("middle_content") << h2("title_top").add(CTemplateSSP::ConvertString("Популярное сейчас")) << DIV("top_list").add(strListHour)).outerHTML();
	
	const string strContentDay = (strLang == "en") ? 
		(DIV("middle_content") << h2("title_top").add("Top sites") << DIV("top_list").add(strListDay)).outerHTML() :
		(DIV("middle_content") << h2("title_top").add(CTemplateSSP::ConvertString("Популярное за сутки")) << DIV("top_list").add(strListDay)).outerHTML();
	
		const string strContentWeek = (strLang == "en") ? 
		(DIV("middle_content") << h2("title_top").add("Top sites") << DIV("top_list").add(strListDay)).outerHTML() :
		(DIV("middle_content") << h2("title_top").add(CTemplateSSP::ConvertString("Популярное за неделю")) << DIV("top_list").add(strListWeek)).outerHTML();

	const string strTabName1 = (strLang == "en") ? "Hour" : CTemplateSSP::ConvertString("Час");
	const string strTabName2 = (strLang == "en") ? "Day" : CTemplateSSP::ConvertString("Сутки");
	const string strTabName3 = (strLang == "en") ? "Week" : CTemplateSSP::ConvertString("Неделя");

	return (DIV("", "tabs-top") << (ul() << (li() << A().href("#tabs-top-1").add(strTabName1)) << (li() << A().href("#tabs-top-2").add(strTabName2)) << (li() <<  A().href("#tabs-top-3").add(strTabName3))) << DIV("", "tabs-top-1").add(strContentHour) << DIV("", "tabs-top-2").add(strContentDay) << DIV("", "tabs-top-3").add(strContentWeek)).outerHTML();
	//if (strLang == "en")
	//	return (DIV("", "tabs-top") << (ul() << (li() << A()) << (li() << A())) << DIV().add(strContentHour) << DIV().add(strContentHour)).outerHTML();
		//return (DIV("ui-widget-content middle_content") << h2("title_top").add("Top sites") << DIV("top_list").add(strList)).outerHTML();
	//if (strLang == "ru")
	//	return (DIV("", "tabs-top") << (ul() << (li() << A()) << (li() << A())) << DIV() << DIV()).outerHTML();
		//return (DIV("ui-widget-content middle_content") << h2("title_top").add(CTemplateSSP::ConvertString("Популярное сейчас")) << DIV("top_list").add(strList)).outerHTML();

}

const string CTopSitesSSP::LeftTopics(const string strLang) const
{
	if (strLang == "en")
	{
		return (
			li("ui-state-default ui-corner-all").add(A("left_link").href("/en/").add("Anonymizer")) <<
			li("ui-corner-all ui-tabs-active ui-state-active").add(span("left_link").add("Top links")) <<
			li("ui-state-default ui-corner-all").add(A("left_link").href("/en/guestbook.ssp").add("Guestbook"))).outerHTML();
	}
	if (strLang == "ru")
	{
		return (
			li("ui-state-default ui-corner-all").add(A("left_link").href("/ru/").add("Анонимайзер")) <<
			li("ui-corner-all ui-tabs-active ui-state-active").add(span("left_link").add("Популярные ссылки")) <<
			li("ui-state-default ui-corner-all").add(A("left_link").href("/ru/guestbook.ssp").add("Гостевая книга"))).outerHTML();
	}
}

const string CTopSitesSSP::GetTitle(const string strLang) const
{
	if (strLang == "en")
		return "Top list of internet sites. Real time update";
	if (strLang == "ru") 
		return ConvertString("Список популярных сейчас сайтов. Обновление в реальном времени");
}

const string CTopSitesSSP::GetDescription(const string strLang) const
{
	if (strLang == "en")
		return "The list of sites that Internet users are browsing right now. To update the list simply refresh the browser";
	if (strLang == "ru") 
		return ConvertString("Список сайтов, которые пользователи интернета просматривают прямо сейчас. Для обновления списка просто обновите окно браузера");
}

const string CTopSitesSSP::GetKeywords(const string strLang) const
{
	if (strLang == "en")
		return "The most recent on the internet, twenty popular at the moment sites, continuous updating";
	if (strLang == "ru") 
		return ConvertString("Самое свежее в интернете, двадцатка популярных в данный момент сайтов, непрерывное обновление");
}
