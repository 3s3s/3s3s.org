#include "GuestBook.h"


#include "../html_framework.h"
#include "Proxy.h"

using namespace proxy_site;
using namespace html;

const string CGuestBookSSP::MiddleContent(const string strLang) const
{
	const string strList = 
		    "<div id=\"hypercomments_widget\"></div>"
			"<script type=\"text/javascript\">"
			"_hcwp = window._hcwp || [];"
			"_hcwp.push({widget:\"Stream\", widget_id: 19685});"
			"(function() {"
			"if(\"HC_LOAD_INIT\" in window)return;"
			"HC_LOAD_INIT = true;"
			"var lang = (navigator.language || navigator.systemLanguage || navigator.userLanguage || \"en\").substr(0, 2).toLowerCase();"
			"var hcc = document.createElement(\"script\"); hcc.type = \"text/javascript\"; hcc.async = true;"
			"hcc.src = (\"https:\" == document.location.protocol ? \"https\" : \"http\")+\"://w.hypercomments.com/widget/hc/19685/\"+lang+\"/widget.js\";"
			"var s = document.getElementsByTagName(\"script\")[0];"
			"s.parentNode.insertBefore(hcc, s.nextSibling);"
			"})();"
			"</script>"
			"<a href=\"http://hypercomments.com\" class=\"hc-link\" title=\"comments widget\">comments powered by HyperComments</a>";
	
	if (strLang == "en")
		return (DIV("ui-widget-content middle_content") << h2("title_top").add("Guestbook") << DIV("top_list").add(strList)).outerHTML();
	if (strLang == "ru")
		return (DIV("ui-widget-content middle_content") << h2("title_top").add(CTemplateSSP::ConvertString("Гостевая книга")) << DIV("top_list").add(strList)).outerHTML();

}

const string CGuestBookSSP::LeftTopics(const string strLang) const
{
	if (strLang == "en")
	{
		return (
			li("ui-state-default ui-corner-all").add(A("left_link").href("/en/").add("Anonymizer")) <<
			li("ui-state-default ui-corner-all").add(A("left_link").href("/en/top.ssp").add("Top links")) << 
			li("ui-corner-all ui-tabs-active ui-state-active").add(span("left_link").add("Guestbook"))).outerHTML();
	}
	if (strLang == "ru")
	{
		return (
			li("ui-state-default ui-corner-all").add(A("left_link").href("/ru/").add("Анонимайзер")) <<
			li("ui-state-default ui-corner-all").add(A("left_link").href("/en/top.ssp").add("Популярные ссылки")) <<
			li("ui-corner-all ui-tabs-active ui-state-active").add(span("left_link").add("Гостевая книга"))).outerHTML();
	}
}

const string CGuestBookSSP::GetTitle(const string strLang) const
{
	if (strLang == "en")
		return "Guestbook";
	if (strLang == "ru") 
		return ConvertString("Книга отзывов и предложений");
}

const string CGuestBookSSP::GetDescription(const string strLang) const
{
	if (strLang == "en")
		return "Tell us what you think about anonymizer sites "  DNS_NAME  ". What do you need? What sites are having problems?";
	if (strLang == "ru") 
		return ConvertString("Расскажите, что вы думаете о разблокировщике сайтов "  DNS_NAME  ". Чего вам не хватает? С какими сайтами возникли проблемы?");
}

const string CGuestBookSSP::GetKeywords(const string strLang) const
{
	if (strLang == "en")
		return "guestbook, forum, comments, suggestions , users anonymizer "  DNS_NAME  ;
	if (strLang == "ru") 
		return ConvertString("Гостевая книга, форум, отзывы, предложения, пользователей анонимайзера "  DNS_NAME );
}
