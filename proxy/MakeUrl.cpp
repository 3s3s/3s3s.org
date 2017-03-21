#include "MakeUrl.h"
#include "../utils/orm.h"

using namespace proxy_site;

const string CMakeUrlSSP::ValidateAlias(const string strAlias) const
{
	string strRet;
	for (size_t n=0; n<strAlias.length(); n++)
	{
		if (strAlias[n]<='Z' && strAlias[n]>='A')
			strRet += strAlias[n];
		if (strAlias[n]<='z' && strAlias[n]>='a')
			strRet += strAlias[n];
		if (strAlias[n]<='9' && strAlias[n]>='0')
			strRet += strAlias[n];
	}

	if (strRet.length() && strRet.rfind('0') == strRet.length()-1)
		strRet += "1";
	return strRet;
}
const string CMakeUrlSSP::ValidateLong(const string strLong) const
{
	if ((strLong.find(".") == -1) || strLong.length() < 4)
		return "";

	const char sz[] = {'-','.','_','~',':','/','?','#','[',']','@','!','$','&','\'','(',')','*','+',',',';','=','%'};
	string strRet;
	for (size_t n=0; n<strLong.length(); n++)
	{
		if (strLong[n]<='Z' && strLong[n]>='A')
			strRet += strLong[n];
		if (strLong[n]<='z' && strLong[n]>='a')
			strRet += strLong[n];
		if (strLong[n]<='9' && strLong[n]>='0')
			strRet += strLong[n];

		for (int i=0; i<23; i++)
		{
			if (sz[i] != strLong[n])
				continue;
			
			strRet += strLong[n];
			break;
		}
	}

	if ((strRet.find("http://") != 0) && (strRet.find("https://") != 0))
		strRet = "http://"+strRet;

	const string strProxy = string(".") + DNS_NAME;

	int nPos = strRet.find(strProxy);
	while (nPos != strRet.npos)
	{
		const string strLeft = strRet.substr(0, nPos);
		const string strRight = strRet.substr(nPos+strProxy.length());

		strRet = strLeft + strRight;

		nPos = strRet.find(strProxy);
	}
	//if (strRet.find("https://") != 0)
	//	strRet = "https://"+strRet;

	return strRet;
}

const string CMakeUrlSSP::GetNewShortLink() const
{
	const char *psz="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	const int nBase = strlen(psz);

	/*orm::CTable count = orm::CTable("LINKS").GetAllTable("COUNT(*) AS cnt");

	size_t nNext = 0;
	if (count.GetRowsCount())
		nNext = 1+atoi(count[0]["cnt"].c_str());*/
	size_t nNext = (time(0)-43*365*24*3600);

	vector<int> vRests;
	//int nRest = nNext;
	while(1)
	{
		int nRest = nNext - (nNext/nBase)*nBase;

		vRests.push_back(nRest);
		if (nNext < nBase)
			break;

		nNext = nNext/nBase;
	}

	string strRet;
	for (int n=vRests.size()-1; n>=0; n--)
	{
		strRet += psz[vRests[n]];
	}
	return strRet;
}

const string CMakeUrlSSP::FillShortLinks(const string strLong, const string strStatus) const
{
	DEBUG_LOG("Try CMakeUrlSSP::FillShortLinks1");

//	DEBUG_LOG("Try CMakeUrlSSP::FillShortLinks2");
	const string strAddition = (strStatus != "temp") ? "" : " AND time IS NOT NULL AND time > 0 AND status='temp'";

	orm::CTable exist = orm::CTable("LINKS").Where("long=\'%s\' COLLATE NOCASE%s", strLong.c_str(), strAddition.c_str()).GetAllTable();
//	DEBUG_LOG("Try CMakeUrlSSP::FillShortLinks3");
	if (!exist.GetRowsCount())
		return "{\"result\": false}";

	ostringstream str;
	str << "{\"result\": true, \"long\": \"" << strLong << "\", \"short\": [\"";

	size_t nMax = exist.GetRowsCount();
	if (nMax > 50) nMax = 50;

//	DEBUG_LOG("Try CMakeUrlSSP::FillShortLinks4 nMax=%i", nMax);
	for (int n=nMax-1; n>=0; n--)
	{
//		DEBUG_LOG("exist[%i][\"short\"]=%s", n, exist[n]["short"].c_str());
		str << exist[n]["short"] << "\"";
		if ((n != 0) && (strStatus != "temp"))
			str << ", \"";

		if (strStatus == "temp")
			break;
	}
	str << "]}";
//	DEBUG_LOG("Try CMakeUrlSSP::FillShortLinks5");
	return str.str();
}

const string CMakeUrlSSP::OnMakeNew(const string strLong, const string strAlias, const string &strIP, const string strStatus, time_t tmCurrent) const
{
	DEBUG_LOG("Try CMakeUrlSSP::OnMakeNew");
	const string strValidLong = ValidateLong(curl::CUrl::URLDecode(strLong));
	if (!strValidLong.length())
		return "{\"result\": false}";

	const string strValidAlias = ValidateAlias(strAlias);

	if (strValidAlias.length())
	{
		orm::CTable exist = orm::CTable("LINKS").Where("creatorIP=\'%s\' AND time+60 > %i", strIP.c_str(), (int)time(NULL)).GetAllTable();
		
//#ifndef _DEBUG
		if (exist.GetRowsCount())
		{
			const string strTimeLast = exist[0][2];
			//DEBUG_LOG("CMakeUrlSSP::Show start");
			return string("{\"result\": false, \"reason\": \"wait24\", \"time_last\": \"") + strTimeLast + "\", \"time\": \"" + to_string(time(NULL)) + "\"}";
		}
//#endif

		orm::CTable("LINKS").InsertOrAbort("\'%s\', \'%s\', %i, \'%s\', \'%s\'", 
			strValidLong.c_str(), strValidAlias.c_str(), (int)tmCurrent, strIP.c_str(), strStatus.c_str());
		orm::CTable("LINKS").InsertOrAbort("\'%s\', \'%s_\', %i, \'%s\', \'%s\'", 
			strValidLong.c_str(), strValidAlias.c_str(), (int)tmCurrent, strIP.c_str(), strStatus.c_str());
	}

	if (strStatus == "temp")
	{
		orm::CTable("LINKS").InsertOrAbort("\'%s\', \'%s.0\', %i, \'%s\', \'%s\'", 
			strValidLong.c_str(), GetNewShortLink().c_str(), (int)tmCurrent, strIP.c_str(), strStatus.c_str());
	}
	else
	{
		orm::CTable exist = orm::CTable("LINKS").Where("long=\'%s\' COLLATE NOCASE", strValidLong.c_str()).GetAllTable();

		if (!exist.GetRowsCount())
		{
			orm::CTable("LINKS").InsertOrAbort("\'%s\', \'%s0\', %i, \'%s\', \'%s\'", 
				strValidLong.c_str(), GetNewShortLink().c_str(), (int)tmCurrent, strIP.c_str(), strStatus.c_str());
		}
	}
	

	return FillShortLinks(strValidLong, strStatus);
}

void CMakeUrlSSP::Show(map<string, string> values, const string strIP, bool bHasGZip, vector<BYTE> *pOut)
{
	DEBUG_LOG("CMakeUrlSSP::Show start");
	if ((values.find("action") == values.end()) ||
		(values.find("long") == values.end()) || 
		(values.find("alias") == values.end()))
	{
		ostringstream str;
		str << "HTTP/1.1 400 Bad Request\r\n" 
			<< "Content-Length: 0\r\n"
			<< "\r\n";

		string strResponce = str.str();
		pOut->resize(strResponce.length());
		memcpy(&((*pOut)[0]), strResponce.c_str(), strResponce.size());
		return;
	}
	
	orm::CTable::ExecuteSQL("DELETE FROM LINKS WHERE long LIKE 'http://https://'");

	const string strAction = values["action"];
	string strRet;
	DEBUG_LOG("CMakeUrlSSP::Show strAction=%s", strAction.c_str());
	if (strAction == "make" || strAction == "make_temp")
	{
		if (strAction == "make_temp")
			strRet = OnMakeNew(orm::utils::ReplaceQuotes(values["long"]), orm::utils::ReplaceQuotes(values["alias"]), strIP, "temp", time(0));
		else
			strRet = OnMakeNew(orm::utils::ReplaceQuotes(values["long"]), orm::utils::ReplaceQuotes(values["alias"]), strIP, "new", time(0));
		
		DEBUG_LOG("OnMakeNew returned strRet=%s", strRet.c_str());
	}

	ostringstream strContent;
	strContent << strRet;//"{\"result\": 200}";
	
	string strUTF8Content = html::utils::ConvertString(strContent.str().c_str());

	ostringstream str;
	str << "HTTP/1.1 200 OK\r\n"
		<< "Content-Type: application/json; charset=utf-8\r\n" 
		<< "Access-Control-Allow-Origin: *\r\n"
		<< "Content-Length: " << strUTF8Content.length() << "\r\n"
		<< "\r\n" << strUTF8Content.c_str();

	string strResponce = str.str();
	pOut->resize(strResponce.length());
	memcpy(&((*pOut)[0]), strResponce.c_str(), strResponce.size());
}