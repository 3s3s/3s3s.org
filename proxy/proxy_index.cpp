#include "index.h"
#include "Template.h"
#include "../html_framework.h"

using namespace proxy_site;
using namespace html;

const string CIndexSSP::CreateShorUrlForm(const string strLang) const
{
	CTable tblCreate(6, 1, "create_form", "create_form");
	
	if (strLang == "ru")
		tblCreate[0][0].insert(DIV().add("<b>������� ������� ������, ����� �������� ��������</b>"));
	else
		tblCreate[0][0].insert(DIV().add("<b>Enter a long link to get short</b>"));

	tblCreate[1][0].insert(input("", "long_url").type("text").size("40"));
	if (strLang == "ru")
	{
		tblCreate[2][0].insert(button("", "make_short_url").add("�������� �������� ������"));
		tblCreate[3][0].insert(DIV().add("�������� ��� (�����������):"));
	}
	else
	{
		tblCreate[2][0].insert(button("", "make_short_url").add("Get a short link"));
		tblCreate[3][0].insert(DIV().add("Desired name (optional):"));
	}

	tblCreate[4][0].insert(tt().add("http://") << input("", "custom_alias").type("text").size("20").maxlength("30") << tt().add("_." DNS_NAME));
	
	if (strLang == "ru")
		tblCreate[5][0].insert("<i>(������ ��������� ������ ����� � ��������� �����)</i>");
	else
		tblCreate[5][0].insert("<i>(Must contain only numbers and letters)</i>");

	tblCreate[1].AddCSS("long_url spaceUnder");
	tblCreate[3].AddCSS("spaceUpper");
	
	//tblCreate[1][0].AddAttribute("style='float:right'");
	tblCreate[1][0].AddAttribute("align='center'");
	tblCreate[2][0].AddAttribute("align='center'");
	tblCreate[4][0].AddAttribute("style='width:100%'");

	return (DIV("ui-widget-content ui-corner-all create_div") << tblCreate).outerHTML();
}

const string CIndexSSP::LeftTopics(const string strLang) const
{
	if (strLang == "en")
	{
		return (
			li("ui-corner-all ui-tabs-active ui-state-active").add(span("left_link").add("Anonymizer")) <<
			li("ui-state-default ui-corner-all").add(A("left_link").href("/en/top.ssp").add("Top links")) <<
			li("ui-state-default ui-corner-all").add(A("left_link").href("/en/guestbook.ssp").add("Guestbook"))).outerHTML();
	}
	if (strLang == "ru")
	{
		return (
			li("ui-corner-all ui-tabs-active ui-state-active").add(span("left_link").add("�����������")) <<
			li("ui-state-default ui-corner-all").add(A("left_link").href("/ru/top.ssp").add("���������� ������")) <<
			li("ui-state-default ui-corner-all").add(A("left_link").href("/ru/guestbook.ssp").add("�������� �����"))).outerHTML();
	}
}


const string CIndexSSP::UnblockSiteForm(const string strLang) const
{
	CTable tblUnblock(5, 1, "unblock_form", "unblock_form");

	if (strLang == "ru")
		tblUnblock[0][0].insert(DIV().add("<b>������� ����� ���������������� �����</b>"));
	else
		tblUnblock[0][0].insert(DIV().add("<b>Enter the address of the blocked site</b>"));

	tblUnblock[1][0].insert(input("", "blocked_url").type("text").size("40"));
	
	if (strLang == "ru")
	{
		tblUnblock[2][0].insert(DIV().add(input("", "encrypt_url").type("checkbox").AddAttribute("checked")).add("����������� �����"));
		tblUnblock[3][0].insert(button("", "go_unblock_url").add("�������"));
		tblUnblock[4][0].insert("<i>(���������������� ���� ��������� � ����� ����)</i>");
	}
	else
	{
		tblUnblock[2][0].insert(DIV().add(input("", "encrypt_url").type("checkbox").AddAttribute("checked")).add("Encrypt address"));
		tblUnblock[3][0].insert(button("", "go_unblock_url").add("Unblock"));
		tblUnblock[4][0].insert("<i>(Unblocked site will open in a new window)</i>");
	}

	
	//tblUnblock[1][0].AddAttribute("style='float:right'");
	tblUnblock[1][0].AddAttribute("align='center'");
	tblUnblock[3][0].AddAttribute("align='center'");

	return (DIV("ui-widget-content ui-corner-all create_div") << tblUnblock).outerHTML();
}

const string CIndexSSP::MiddleContent(const string strLang) const
{
	if (strLang == "en")
	{
		return (DIV("middle_content", "accordion") <<
					h2("middle_title").add("Sites Unblocker and Anonymizer") <<
					(DIV("", "main_info") <<
						h3("middle_title2").add("View blocked site") <<
						DIV("middle_text").style("display:block").add(
							"<p>Your favorite site someone has blocked? You do not like censorship?<br>"
							"Enter the site address in the text box below and click \"Unblock\".<br>") <<
						DIV("middle_text").style("display:block").add(DIV("select_border width_500").add(UnblockSiteForm("en")))) <<
						h2("middle_title").add("Shortlinks service") <<
						(DIV("", "main_info") <<
							h3("middle_title2").add("Short links for redirects and anonymous views") <<
							DIV("middle_text").style("display:block").add(
								"<p>You want to add a link to the forum or twitter, but the link is very long and not fully inserted?"
								"For this invented shortlinks and redirection services. Insert your long link into the text box and get her a short version. "
								"Validity of short links is <b>unlimited.</b>") <<
							(DIV("middle_text").style("display:block").add(DIV("select_border").add(CreateShorUrlForm("en"))) <<
							DIV("middle_text", "short_links")))).outerHTML();
	}
	if (strLang == "ru")
	{
		return ConvertString((DIV("middle_content", "accordion") <<
					h2("middle_title").add("������������� ������") <<
					(DIV("", "main_info") <<
						h3("middle_title2").add("������� �� ��������������� ����") <<
						DIV("middle_text").style("display:block").add(
							"<p>�� ������ ����� �� ����, � ��� ������������ �������� � �������: <b>���� ������������</b>?<br>"
							"��� �� ��������, ��� ��� ������� ���� ���-�� ���������? ������� ����� ����� � ��������� ����, ������������� ���� � "
							"������� ������ \"�������\".<br>") <<
						DIV("middle_text").style("display:block").add(DIV("select_border width_500").add(UnblockSiteForm("ru")))) <<
						h2("middle_title").add("������ �������� ������") <<
						(DIV("", "main_info") <<
							h3("middle_title2").add("�������� ������ ��� ���������� � ��������� ����������") <<
							DIV("middle_text").style("display:block").add(
								"<p>�� ������ �������� ��������-������ �� ����� ��� � �������, �� ������ ����� ������� � ����������� �� ���������?"
								"��� ����� ��������� ������� �������� ������ ��� ���������. �������� ���� ������� ������ � ��������� ���� � �������� �� "
								"�������� �������. ���� �������� �������� ������ <b>�����������.</b>") <<
							(DIV("middle_text").style("display:block").add(DIV("select_border").add(CreateShorUrlForm("ru"))) <<
							DIV("middle_text", "short_links")))).outerHTML());
	}
}

const string CIndexSSP::GetTitle(const string strLang) const
{
	if (strLang == "en")
		return "Simple and easy to use anonymizer";
	if (strLang == "ru")
		return ConvertString("������� � ������� �����������");
}

const string CIndexSSP::GetDescription(const string strLang) const
{
	if (strLang == "en")
		return "Go to a blocked site can be easily and simply. Add to the end of the address '."  DNS_NAME  "' and site will unblocked. Enter the site address in the text box and click 'Unblock' key.";
	if (strLang == "ru")
		return ConvertString("����� �� ��������������� ���� ����� ����� � ������, ������� � ����� ������ ������ '."  DNS_NAME  "'. ������� ����� ���������������� ����� � ��������� ���� � ������� �� ������ '�������'");
}

const string CIndexSSP::GetKeywords(const string strLang) const
{
	if (strLang == "en")
		return "Easiest anonymizer and shortlinks service. Unblock any sites in the world quick and easy. No proxy. No plugins or addons. Web browser only!";
	if (strLang == "ru")
		return ConvertString("���������� ����������� ��� ��������������� ������, ������ �������� ������, ����������, ������� �������� ������");
}
