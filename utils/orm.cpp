/*#ifdef _DEBUG
#include <iostream>
#include <map>
#include <afx.h>
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif*/
#include "orm.h"
//#include "../html_framework.h"



using namespace orm;

sqlite3 *CTable::gpDataBase = NULL;
bool CTable::gTransactionState = false;
sqlite3_mutex *utils::CMutex::m_pMutex = 0;

const CTable CTable::CreateTable(const string &strTable, const string &strQuery)
{
	DEBUG_LOG("//////////////////////////////////////////////////");
	DEBUG_LOG("Create table %s in database", strTable.c_str());
	
	char *szErrMsg = 0;

	string strSQL = "CREATE TABLE IF NOT EXISTS " + strTable + " " + strQuery;
	int rc = sqlite3_exec(gpDataBase, strSQL.c_str(), 0, 0, &szErrMsg);
			
	time_t tmRet = 0;
	if( rc != SQLITE_OK )
	{
#ifdef _DEBUG
//		::MessageBoxA(0, szErrMsg, strSQL.c_str(), MB_OK);
#endif
		DEBUG_LOG("CreateTable !!! Database Error !!! (str=%s) sql=%s", szErrMsg, strSQL.c_str());
		sqlite3_free(szErrMsg);
	}
	else
	{
		tmRet = time(NULL);
		DEBUG_LOG("All Updated CTable::CreateTable");
	}
	return CTable(strTable);
}

const CTable CTable::InsertOrReplace(char const* fmt, ...) const
{
    va_list ap;
    va_start(ap, fmt);
    const string strValues = utils::dupvprintf(fmt, ap);
    va_end(ap);

	string strSQL = "INSERT OR REPLACE INTO  " + m_strTableName + " VALUES (" + strValues + ")";

	char *szErrMsg = 0;
	int rc = sqlite3_exec(gpDataBase, strSQL.c_str(), 0, 0, &szErrMsg);
			
	time_t tmRet = 0;
	if( rc != SQLITE_OK )
	{
#ifdef _DEBUG
		//::MessageBoxA(0, strSQL.c_str(), szErrMsg, MB_OK);
#endif
		DEBUG_LOG("InsertOrReplace !!! Database Error !!! (str=%s) sql=%s", szErrMsg, strSQL.c_str());
		sqlite3_free(szErrMsg);
	}
	else
	{
		tmRet = time(NULL);
		DEBUG_LOG("All Updated CTable::InsertOrReplace");
	}
	DEBUG_LOG("//////////////////////////////////////////////////");
	return (*this);
}

const CTable CTable::InsertOrAbort(char const* fmt, ...) const
{
    va_list ap;
    va_start(ap, fmt);
    const string strValues = utils::dupvprintf(fmt, ap);
    va_end(ap);

	string strSQL = "INSERT OR ABORT INTO " + m_strTableName + " VALUES (" + strValues + ")";

	char *szErrMsg = 0;
	int rc = sqlite3_exec(gpDataBase, strSQL.c_str(), 0, 0, &szErrMsg);
			
	time_t tmRet = 0;
	if( rc != SQLITE_OK )
	{
#ifdef _DEBUG
//		::MessageBoxA(0, strSQL.c_str(), szErrMsg, MB_OK);
#endif
		DEBUG_LOG("InsertOrAbort !!! Database Error !!! (str=%s) sql=%s", szErrMsg, strSQL.c_str());
		sqlite3_free(szErrMsg);
	}
	else
	{
		tmRet = time(NULL);
		DEBUG_LOG("All Updated CTable::InsertOrAbort");
	}
	DEBUG_LOG("//////////////////////////////////////////////////");
	return (*this);
}

const CTable CTable::Insert(char const* fmt, ...) const
{
    va_list ap;
    va_start(ap, fmt);
    const string strValues = utils::dupvprintf(fmt, ap);
    va_end(ap);

	string strSQL = "INSERT INTO  " + m_strTableName + " VALUES (" + strValues + ")";

	char *szErrMsg = 0;
	int rc = sqlite3_exec(gpDataBase, strSQL.c_str(), 0, 0, &szErrMsg);
			
	time_t tmRet = 0;
	if( rc != SQLITE_OK )
	{
#ifdef _DEBUG
//		::MessageBoxA(0, strSQL.c_str(), szErrMsg, MB_OK);
#endif
		DEBUG_LOG("Insert !!! Database Error !!! (str=%s) sql=%s", szErrMsg, strSQL.c_str());
		sqlite3_free(szErrMsg);
	}
	else
	{
		tmRet = time(NULL);
		DEBUG_LOG("All Updated CTable::Insert");
	}
	DEBUG_LOG("//////////////////////////////////////////////////");
	return (*this);
}

const CTable CTable::Where(char const* fmt, ...) const
{
    va_list ap;
    va_start(ap, fmt);
    const string ret = utils::dupvprintf(fmt, ap);
    va_end(ap);

	return CTable(m_strTableName, m_strQuery+" WHERE "+ret);
}

const CTable CTable::OrderBy(char const* fmt, ...) const
{
    va_list ap;
    va_start(ap, fmt);
    const string ret = utils::dupvprintf(fmt, ap);
    va_end(ap);

	return CTable(m_strTableName, m_strQuery+" ORDER BY "+ret);
}

const CTable CTable::And(char const* fmt, ...) const
{
    va_list ap;
    va_start(ap, fmt);
    const string ret = utils::dupvprintf(fmt, ap);
    va_end(ap);

	return CTable(m_strTableName, m_strQuery+" AND "+ret);
}

const CTable CTable::Set(char const* fmt, ...) const
{
    va_list ap;
    va_start(ap, fmt);
    const string ret = utils::dupvprintf(fmt, ap);
    va_end(ap);

	return CTable(m_strTableName, m_strQuery+" SET "+ret);//GetAllTable("WHERE "+ret);
}

const CTable CTable::GetAllTableDistinct(const string &strDistinctColumns) const
{
	assert(gpDataBase);
	assert(m_strTableName.length());

	if (m_strQuery.find("SET ") != m_strQuery.npos)
		return Update().GetAllTable();


	DEBUG_LOG("//////////////////////////////////////////////////");
	DEBUG_LOG("Get table %s from database", m_strTableName.c_str());

	class CLocal
	{
	public:
		static int Callback(void *lParam, int nColumnCount, char **ppszColTextArray, char **azColName)
		{
			CTable *pTable = (CTable *)lParam;

			pTable->AddRow(nColumnCount, ppszColTextArray, azColName);
			return 0;
		}
	};

	char *szErrMsg = 0;

	string strSQL = "SELECT DISTINCT "+strDistinctColumns+" FROM " + m_strTableName + " " + m_strQuery;

	CTable ret(m_strTableName);
	int rc = sqlite3_exec(gpDataBase, strSQL.c_str(), CLocal::Callback, &ret, &szErrMsg);
			
	if( rc != SQLITE_OK )
	{
		DEBUG_LOG("GetAllTableDistinct !!! Database Error !!! (str=%s) sql=%s", szErrMsg, strSQL.c_str());
		sqlite3_free(szErrMsg);
	}
	else
	{
		DEBUG_LOG("All Updated CTable::GetAllTableDistinct");
	}
	DEBUG_LOG("//////////////////////////////////////////////////");
	return ret;
}

const string CTable::ToJSON() const
{
	string strRet = "[";
	for (size_t n=0; n<m_pRows->size(); n++)
	{
		strRet += "{";
		const size_t nCollsCount = m_pRows->at(n).GetColsCount();
		for (size_t m=0; m<nCollsCount; m++)
		{
			strRet += "\""+GetColName(m)+"\": \""+m_pRows->at(n)[m]+"\"";
			if (m != nCollsCount-1)
				strRet += ", ";
		}
		strRet += "}";

		if (n != m_pRows->size()-1)
			strRet += ", ";
	}
	strRet += "]";
	return strRet;
}

const CTable CTable::GetAllTable(const string strSelect) const
{
	assert(gpDataBase);
	assert(m_strTableName.length());

	if (m_strQuery.find("SET ") != m_strQuery.npos)
		return Update().GetAllTable();


	//DEBUG_LOG("//////////////////////////////////////////////////");
	//DEBUG_LOG("Get table %s from database", m_strTableName.c_str());

	class CLocal
	{
	public:
		static int Callback(void *lParam, int nColumnCount, char **ppszColTextArray, char **azColName)
		{
			CTable *pTable = (CTable *)lParam;

			pTable->AddRow(nColumnCount, ppszColTextArray, azColName);
			return 0;
		}
	};

	char *szErrMsg = 0;

	string strSQL = "SELECT " + strSelect + " FROM " + m_strTableName + " " + m_strQuery;

	CTable ret(m_strTableName);
	int rc = sqlite3_exec(gpDataBase, strSQL.c_str(), CLocal::Callback, &ret, &szErrMsg);
			
	if( rc != SQLITE_OK )
	{
		DEBUG_LOG("GetAllTable !!! Database Error !!! (str=%s) sql=%s", szErrMsg, strSQL.c_str());
		sqlite3_free(szErrMsg);
	}
	else
	{
		//DEBUG_LOG("All Updated");
	}
	//DEBUG_LOG("//////////////////////////////////////////////////");
	return ret;
}

const CTable CTable::Update() const
{
	DEBUG_LOG("Try Update table %s", m_strTableName.c_str());
	string strSQL = "UPDATE " + m_strTableName + " " + m_strQuery;
	
	char *szErrMsg = 0;
	int rc = sqlite3_exec(gpDataBase, strSQL.c_str(), 0, 0, &szErrMsg);
			
	time_t tmRet = 0;
	if( rc != SQLITE_OK )
	{
		DEBUG_LOG("Update !!! Database Error !!! (str=%s) sql=%s", szErrMsg, strSQL.c_str());
		sqlite3_free(szErrMsg);
	}
	else
	{
		tmRet = time(NULL);
		DEBUG_LOG("All Updated CTable::Update");
	}
	DEBUG_LOG("//////////////////////////////////////////////////");
	return CTable(m_strTableName);
}

/*void CTable::UpdateRow(const string strSet, const string strWhere) const
{
	DEBUG_LOG("Try Update row in table %s", m_strTableName.c_str());
	string strSQL = "UPDATE " + m_strTableName+ " SET " + strSet + " WHERE " + strWhere;
	
	char *szErrMsg = 0;
	int rc = sqlite3_exec(gpDataBase, strSQL.c_str(), 0, 0, &szErrMsg);
			
	time_t tmRet = 0;
	if( rc != SQLITE_OK )
	{
		sqlite3_free(szErrMsg);
		DEBUG_LOG("!!! Database Error !!!");
	}
	else
	{
		tmRet = time(NULL);
		DEBUG_LOG("All Updated");
	}
	DEBUG_LOG("//////////////////////////////////////////////////");
}*/

void CTable::InsertRow(CRow row) const
{
	DEBUG_LOG("Try Insert row to table %s", m_strTableName.c_str());
	
	row.SetCollNames(m_pColNames);

	string strValues = "('";
	for (size_t n=0; n<row.GetColsCount()-1; n++)
	{
		strValues += row[n] + "', '";
	}
	strValues += row[row.GetColsCount()-1]+"')";

	string strSQL = "INSERT INTO  " + m_strTableName + " VALUES " + strValues;

	char *szErrMsg = 0;
	int rc = sqlite3_exec(gpDataBase, strSQL.c_str(), 0, 0, &szErrMsg);
			
	time_t tmRet = 0;
	if( rc != SQLITE_OK )
	{
		DEBUG_LOG("InsertRow !!! Database Error !!! (str=%s) sql=%s", szErrMsg, strSQL.c_str());
		sqlite3_free(szErrMsg);
	}
	else
	{
		tmRet = time(NULL);
		DEBUG_LOG("All Updated CTable::InsertRow");
	}
	DEBUG_LOG("//////////////////////////////////////////////////");
}
