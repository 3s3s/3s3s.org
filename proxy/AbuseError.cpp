#include "AbuseError.h"


#include "../html_framework.h"
#include "Proxy.h"
#include "../log.h"

using namespace proxy_site;
using namespace html;

const string CAbuseErrorSSP::MiddleContent(const string strLang) const
{
	DEBUG_LOG("CAbuseErrorSSP::MiddleContent referrer=%s", GetValue("referer").c_str());

	const string strTextRu = 
			string("<div>�� ��������� ������������ ����������� ") +  DNS_NAME + " ��� ����, ����� ����� �� ����: <br><br>"
			"<span class='bad_link'>"+GetValue("referer")+"</span><br><br>"+
			"<b>������ ���������!</b> �������� ��������� ����� ����� ������� �� ��������������! <br><br>"
			"�������������: ���-������������ (� ��� ����� � "  + DNS_NAME  + ") �� ����������� ��� ������ �����������. ��� ������� ������������ ����������� ������-������� ��� VPN-�������.<br><br>"
			"�� �� ����� ����� �� ���� ��������������� �� �������� ��������� ������, �� �� ������ ������������ ����� ������ ���������� �����������, �������� <a href='http://anonymouse.org/'>http://anonymouse.org/</a>"
			"</div>";

	const string strTextEN = 
			string("<div>You are trying to use anonymizer ")  + DNS_NAME  + " to go to the site: <br><br>"
			"<span class='bad_link'>"+GetValue("referer")+"</span><br><br>"+
			"<b>Be careful!</b> Perhaps the owners of this site are spying on users! <br><br>"
			"WARNING: Web anonymizers (including "  + DNS_NAME  + ") does not guarantee you the full anonymity. For extra safety, use proxy servers or VPN-servers.<br><br>"
			"We do not want to take responsibility for the actions of third-party sites, but you can use any other popular anonymizer, for example <a href='http://anonymouse.org/'>http://anonymouse.org/</a>"
			"</div>";
	
	if (strLang == "en")
		return (DIV("ui-widget-content middle_content") << h2("title_top").add("Redirect error") << DIV("top_list").add(strTextEN)).outerHTML();
	if (strLang == "ru")
		return (DIV("ui-widget-content middle_content") << h2("title_top").add(CTemplateSSP::ConvertString("������ ���������������")) << DIV("top_list").add(CTemplateSSP::ConvertString(strTextRu))).outerHTML();

}

const string CAbuseErrorSSP::LeftTopics(const string strLang) const
{
	if (strLang == "en")
	{
		return (
			li("ui-state-default ui-corner-all").add(A("left_link").href("/en/").add("Anonymizer")) <<
			li("ui-state-default ui-corner-all").add(A("left_link").href("/en/top.ssp").add("Top links")) << 
			li("ui-state-default ui-corner-all").add(A("left_link").href("/en/guestbook.ssp").add("Guestbook"))).outerHTML();
	}
	if (strLang == "ru")
	{
		return (
			li("ui-state-default ui-corner-all").add(A("left_link").href("/ru/").add("�����������")) <<
			li("ui-state-default ui-corner-all").add(A("left_link").href("/ru/top.ssp").add("���������� ������")) <<
			li("ui-state-default ui-corner-all").add(A("left_link").href("/ru/guestbook.ssp").add("�������� �����"))).outerHTML();
	}
}

const string CAbuseErrorSSP::GetTitle(const string strLang) const
{
	if (strLang == "en")
		return "Redirect error page";
	if (strLang == "ru") 
		return ConvertString("�������������� � ���������������");
}

const string CAbuseErrorSSP::GetDescription(const string strLang) const
{
	return GetTitle(strLang);
}

const string CAbuseErrorSSP::GetKeywords(const string strLang) const
{
	return GetTitle(strLang);
}
