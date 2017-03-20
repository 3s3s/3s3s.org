#include "Template.h"
#include "../html_framework.h"

using namespace proxy_site;
using namespace html;

const string CTemplateSSP::ConvertString (const string strASCII) const
{
	return utils::ConvertString(strASCII.c_str());
}

void CTemplateSSP::ContinueShow(vector<BYTE> *pOut, string strUTF8Content, bool bHasGZip) const
{
	//string strUTF8Content = ConvertString(pContent->str());
	vector<BYTE> vOut;

	string strGzip = "";
	if (bHasGZip)
	{
		strGzip = "Content-Encoding: gzip\r\n";
		utils::compress_memory((void *)strUTF8Content.c_str(), strUTF8Content.length(), vOut);
		strUTF8Content = (const char *)&vOut[0];
	}
	else
	{
		vOut.resize(strUTF8Content.length());
		memcpy((void *)&vOut[0], strUTF8Content.c_str(), strUTF8Content.length());
	}

	ostringstream str;
	str << "HTTP/1.1 200 OK\r\n"
		<< "Content-Type: text/html; charset=utf-8\r\n"
		<< "Content-Length: " << vOut.size() << "\r\n" <<
		strGzip <<
		"\r\n";

	string strResponce = str.str();
	pOut->resize(strResponce.size()+vOut.size());
	memcpy(&((*pOut)[0]), strResponce.c_str(), strResponce.size());
	memcpy(&((*pOut)[strResponce.size()]), &vOut[0], vOut.size());
}

const string CTemplateSSP::RightContent(const string strLang) const
{
	return 
			"<script async src=\"//pagead2.googlesyndication.com/pagead/js/adsbygoogle.js\"></script>"
			"<!-- big_right -->"
			"<ins class=\"adsbygoogle\""
				 "style=\"display:inline-block;width:300px;height:600px\""
				 "data-ad-client=\"ca-pub-9472318620093072\""
				 "data-ad-slot=\"7606054144\"></ins>"
			"<script>"
			"(adsbygoogle = window.adsbygoogle || []).push({});"
			"</script>";
}

const string CTemplateSSP::LeftContent(const string strLang) const
{
	if (strLang == "en")
	{
		return 
			(DIV("ui-tabs ui-tabs-vertical") <<
				//DIV("leftHeader").add("Topics:") <<
				ul("left_content ui-tabs-nav ui-widget-header ui-corner-all").add(LeftTopics(strLang))
			).outerHTML();
	}
	if (strLang == "ru")
	{
		return ConvertString(
			(DIV("ui-tabs ui-tabs-vertical") <<
				//DIV("leftHeader").add("Разделы:") <<
				ul("left_content ui-tabs-nav ui-widget-header ui-corner-all").add(LeftTopics(strLang))
			).outerHTML());
	}
}

const string CTemplateSSP::ContentHeader(const string strLang)
{
	string strFlags;
	if (strLang == "ru")
	{
		strFlags =
					td().AddAttribute("width='100%' valign='middle' height='2' align='right'").add
					(
						span("lang_flag_image").add
							(IMG().style("width='16' height='11' border='0'").src("/img/flag_ru.jpg"))
					).add
					(
						span("lang_link_text").add("<b>RU</b>").add("<span>&nbsp</span>")
					).add
					(
						A("lang_flag_image").href("/en/").add
							(IMG().style("width='16' height='11' border='0'").src("/img/flag_eng.jpg"))
					).add
					(
						A("lang_link_text").href("/en/").add("EN").add("<span>&nbsp</span>")
					).outerHTML();

	}
	else
	{
		strFlags =
					td().AddAttribute("width='100%' valign='middle' height='2' align='right'").add
					(
						A("lang_flag_image").href("/ru/").add
							(IMG().style("width='16' height='11' border='0'").src("/img/flag_ru.jpg"))
					).add
					(
						A("lang_link_text").href("/ru/").add("RU").add("<span>&nbsp</span>")
					).add
					(
						span("lang_flag_image").add
							(IMG().style("width='16' height='11' border='0'").src("/img/flag_eng.jpg"))
					).add
					(
						span("lang_link_text").add("<b>EN</b>").add("<span>&nbsp</span>")
					).outerHTML();
	}
	return 
		DIV("content_header").add
		( 
			table().style("width:100%; cellspacing='0' cellpadding='0' border='0'").add
			(
				tr().add
				(
					td().AddAttribute("valign='top' height='2' align='left' colspan='2'")
				).add
				(
					strFlags
				)
			)
		).outerHTML();
}

const string CTemplateSSP::StartBody() const
{
	return
			string("<script>"
			  "(function(i,s,o,g,r,a,m){i['GoogleAnalyticsObject']=r;i[r]=i[r]||function(){"
			  "(i[r].q=i[r].q||[]).push(arguments)},i[r].l=1*new Date();a=s.createElement(o),"
			  "m=s.getElementsByTagName(o)[0];a.async=1;a.src=g;m.parentNode.insertBefore(a,m)"
			  "})(window,document,'script','//www.google-analytics.com/analytics.js','ga');"

			  "ga('create', 'UA-59941076-1', 'auto');"
			  "ga('send', 'pageview');"

			"</script>") +
		script().AddAttribute("type=\"text/javascript\" src=\"//yandex.st/share/share.js\" charset=\"utf-8\"").outerHTML();
		/*(DIV("", "fb-root") <<
		//twitter
		script().add(
			"(function(d, s, id) {"
			  "var js, fjs = d.getElementsByTagName(s)[0];"
			  "if (d.getElementById(id)) return;"
			  "js = d.createElement(s); js.id = id;"
			  "js.src = \"//connect.facebook.net/ru_RU/sdk.js#xfbml=1&version=v2.0\";"
			  "fjs.parentNode.insertBefore(js, fjs);"
			  "}(document, 'script', 'facebook-jssdk'));"		
		) <<
		//facebook
		script().add(
		"!function(d,s,id){var js,fjs=d.getElementsByTagName(s)[0];if(!d.getElementById(id)){js=d.createElement(s);js.id=id;js.src=\"https://platform.twitter.com/widgets.js\";fjs.parentNode.insertBefore(js,fjs);}}(document,\"script\",\"twitter-wjs\");"
		)).outerHTML();*/
}

void CTemplateSSP::ShowEN(map<string, string> values, const string strIP, bool bHasGZip, vector<BYTE> *pOut)
{
#ifdef _DEBUG
	values["debug"] = "test2";
#endif
	m_Values = values;
	m_strIP = strIP;

	ostringstream strContent;
	strContent << 
		"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n" <<
		(HTML() << 
			(head() << 
				title().add(GetTitle("en")) <<
				html::link().rel("stylesheet").type("text/css").href(string("//") +  DNS_NAME + "/style.css") <<
				html::link().rel("stylesheet").type("text/css").href("//ajax.googleapis.com/ajax/libs/jqueryui/1.11.1/themes/redmond/jquery-ui.css") <<
				meta().http_equiv("Content-Type").content("text/html; charset=utf-8") << 
				meta().name("description").content(GetDescription("en")) <<
				meta().name("keywords").content(GetKeywords("en")) <<
				meta().name("google-site-verification").content("OsQRSxgdOfBQ9-6da1uGwP33IRPRJx2SXG-0evqDFiQ") <<
				meta().prop("fb:admins").content("100007835598222") <<
				script().src("//ajax.googleapis.com/ajax/libs/jquery/3.1.1/jquery.min.js") <<
				script().src("/jscripts/jquery-ui-1.10.3.min.js") <<
				script().src("/jscripts/makeurl.js")) << 
			(body().add(StartBody()) <<
				(DIV("main_page") << 
					(DIV("main_sheet") << 
						(DIV("main_header") << 
							h1("logoh1").add("3S - Stay Safe Surfing") <<
							h3("logoh3").add("Internet a zone of anonymity and freedom!")
						) <<
						(DIV("main_content").add(
							CTemplateSSP::ContentHeader("en")) <<//DIV("content_header") <<
							(DIV("content_middle") << 
								(table().style("width:100%; table-layout: fixed; cellspacing='0' cellpadding='0' align='center'") <<
									(tr() <<
										td("middle_left").add(LeftContent("en")) <<
										td("middle_middle").add(MiddleContent("en")) <<
										td("middle_right").add(RightContent("en"))
									)
								)
							) <<
							DIV("content_footer")
						)) << 
					DIV("main_footer").add(
table().style("width:100%; cellspacing='0' cellpadding='0' align='center'").add(tr() << 
td().add(
"" ) << 
td().style("text-align: right").add(
"<a onclick=\"myFunction()\"><img src=\"https://sourceforge.net/p/sms-messenger/screenshot/bitcoindonate.png\" width=\"176\" height=\"72\" alt=\"Read book\"></a>\n"
"<p id=\"demo\"></p>\n"
"<script>\n"
"function myFunction()\n"
"{\n"
"var x;\n"
"var person=prompt(\"To donate, please copy-paste my BitCoin address to your BitCoin software.\",\"1CHeYxfYo6zVmHSm7B1KztA5f7ZKcMsEWA\");\n"
"if (person!=null)\n"
  "{\n"
  "x=\"Thank you for donation!\";\n"
  "document.getElementById(\"demo\").innerHTML=x;\n"
  "}\n"
"}\n"
"</script>\n"
))))
			)).outerHTML();

	CTemplateSSP::ContinueShow(pOut, strContent.str(), bHasGZip);
}

#ifdef _WIN32
#define MAIN_CSS	"style.css"
#else
#define MAIN_CSS	string("//") +  DNS_NAME +  "/style.css"
#endif

void CTemplateSSP::Show(map<string, string> values, const string strIP, bool bHasGZip, vector<BYTE> *pOut)
{
#ifdef _DEBUG
	values["debug"] = "test1";
#endif
	m_Values = values;
	m_strIP = strIP;

	ostringstream strContent;
	strContent << 
		"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n" <<
		(HTML() << 
			(head() << 
				title().add(GetTitle("ru")) <<
				html::link().rel("stylesheet").type("text/css").href(MAIN_CSS) <<
				html::link().rel("stylesheet").type("text/css").href("//ajax.googleapis.com/ajax/libs/jqueryui/1.11.1/themes/redmond/jquery-ui.css") << //("/jscripts/jquery-ui-1.10.3/css/redmond/jquery-ui-1.10.3.min.css") <<
				meta().http_equiv("Content-Type").content("text/html; charset=utf-8") << 
				meta().name("description").content(GetDescription("ru")) <<
				meta().name("keywords").content(GetKeywords("ru")) <<
				meta().name("google-site-verification").content("OsQRSxgdOfBQ9-6da1uGwP33IRPRJx2SXG-0evqDFiQ") <<
				meta().prop("fb:admins").content("100007835598222") <<
				script().src("//ajax.googleapis.com/ajax/libs/jquery/3.1.1/jquery.min.js") <<
				script().src("/jscripts/jquery-ui-1.10.3.min.js") <<
				script().src("/jscripts/makeurl.js")) << 
			(body().add(StartBody()) <<
				(DIV("main_page") << 
					(DIV("main_sheet") << 
						(DIV("main_header") << 
							h1("logoh1").add("3S - Stay Safe Surfing") <<
							h3("logoh3").add(ConvertString("Чувствуйте себя в интернете комфортно!"))
						) <<
						(DIV("main_content").add(
							CTemplateSSP::ContentHeader("ru")) <<//DIV("content_header") <<
							(DIV("content_middle") << 
								(table().style("width:100%; table-layout: fixed; cellspacing='0' cellpadding='0' align='center'") <<
									(tr() <<
										td("middle_left").add(LeftContent("ru")) <<
										td("middle_middle").add(MiddleContent("ru")) <<
										td("middle_right").add(RightContent("ru"))
									)
								)
							) <<
							DIV("content_footer")
						)) << 
					DIV("main_footer").add(
table().style("width:100%; cellspacing='0' cellpadding='0' align='center'").add(tr() << 
td().add(ConvertString(
"<!--LiveInternet counter--><script type=\"text/javascript\"><!--\r\n"
"document.write(\"<a href='http://www.liveinternet.ru/click' \"+\r\n"
"\"target=_blank><img src='//counter.yadro.ru/hit?t22.10;r\"+\r\n"
"escape(document.referrer)+((typeof(screen)==\"undefined\")?\"\":\r\n"
"\";s\"+screen.width+\"*\"+screen.height+\"*\"+(screen.colorDepth?\r\n"
"screen.colorDepth:screen.pixelDepth))+\";u\"+escape(document.URL)+\r\n"
"\";\"+Math.random()+\r\n"
"\"' alt='' title='LiveInternet: показано число просмотров за 24\"+\r\n"
"\" часа, посетителей за 24 часа и за сегодня' \"+\r\n"
"\"border='0' width='88' height='31'><\\/a>\")\r\n"
"//--></script><!--/LiveInternet-->\r\n" )) << 
td().style("text-align: right").add(
"<a onclick=\"myFunction()\"><img src=\"https://sourceforge.net/p/sms-messenger/screenshot/bitcoindonate.png\" width=\"176\" height=\"72\" alt=\"Read book\"></a>\n"
"<p id=\"demo\"></p>\n"
"<script>\n"
"function myFunction()\n"
"{\n"
"var x;\n"
"var person=prompt(\"To donate, please copy-paste my BitCoin address to your BitCoin software.\",\"1CHeYxfYo6zVmHSm7B1KztA5f7ZKcMsEWA\");\n"
"if (person!=null)\n"
  "{\n"
  "x=\"Thank you for donation!\";\n"
  "document.getElementById(\"demo\").innerHTML=x;\n"
  "}\n"
"}\n"
"</script>\n"
))))
			)).outerHTML();

	CTemplateSSP::ContinueShow(pOut, strContent.str(), bHasGZip);
}