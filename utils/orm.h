#ifndef _ORM_
#define _ORM_
#include "../sqlite/sqlite3.h"
#include <assert.h>
#include <time.h>
#include <sstream>
#include <string>
#include <vector>
#include <memory>
#include "../log.h"

using namespace std;

namespace orm
{
	class utils
	{
	public:
		class CMutex
		{
			static sqlite3_mutex *m_pMutex;
			int m_nError;
		public:
			CMutex(bool bBlock = true) : m_nError(0)
			{
				if (!m_pMutex)
				{
					m_pMutex = sqlite3_mutex_alloc(SQLITE_MUTEX_FAST);

					if (!m_pMutex)
						return;
				}

				if (bBlock)
					sqlite3_mutex_enter(m_pMutex);
				else
					m_nError = sqlite3_mutex_try(m_pMutex);
			}
			~CMutex()
			{
				if (!m_pMutex)
					return;
				sqlite3_mutex_leave(m_pMutex);
			}
		};
		static const string dupvprintf(const char *fmt, va_list ap)
		{
			static vector<char> buf(1000000);
			int len, size;

			//buf = (char *)malloc(1000000);
			//buf.resize(1000000);
			size = buf.size();

			while (1) {
        #ifdef CODEBLOKSCODEBLOKS
            len = vsnprintf(&buf[0], size-1, fmt, ap);
        #else
		#ifdef WIN32
		//#define vsnprintf _vsnprintf
			len = _vsnprintf_s(&buf[0], size-1, size-1, fmt, ap);
		#else
			len = vsnprintf(&buf[0], size-1, fmt, ap);
		#endif
		#endif
			//buf.push_back(0);
			if (len >= 0 && len < size-1)
			{
				/* This is the C99-specified criterion for snprintf to have
				 * been completely successful. */
				buf[size-1] = 0;
				return &buf[0];
			} else if (len > 0) {
				/* This is the C99 error condition: the returned length is
				 * the required buffer size not counting the NUL. */
				size = len + 1;
			} else {
				/* This is the pre-C99 glibc error condition: <0 means the
				 * buffer wasn't big enough, so we enlarge it a bit and hope. */
				size += 1000000;
			}
			//buf = (char *)realloc(buf, size);//sresize(buf, size, char);
			buf.resize(size);
			}
			buf[0] = 0;
			return &buf[0];
		}
		static const string TimeToString(const time_t t)
		{
			ostringstream str;
			str << t;
			return str.str();
		}
		static const string ReplaceQuotes(const string sqlString)
		{
			string strRet;

			string strTemp = sqlString;
			int nPos = strTemp.find("'");
			while(nPos != strTemp.npos)
			{
				strRet += strTemp.substr(0, nPos)+"''";

				if (nPos == strTemp.length())
					break;

				string str = strTemp.substr(nPos+1);
				strTemp = str;
				nPos = strTemp.find("'");

				if (nPos == strTemp.npos)
				{
					strRet += str;
					break;
				}
			}

			if (!strRet.length())
				strRet = sqlString;

			return strRet;
		}
	};
	class CRow
	{
		shared_ptr<vector<string> > m_pValues;
	public:
		shared_ptr<vector<string> > m_pNames;
		CRow(int nColumnCount, char **ppszColTextArray, const vector<string> &vNames) :
		  m_pNames(new vector<string>(vNames)), m_pValues(new vector<string>)
		{
			for (int n=0; n<nColumnCount; n++)
			{
				if (ppszColTextArray[n])
					m_pValues->push_back(ppszColTextArray[n]);
				else
					m_pValues->push_back("");
			}
		}
		CRow(vector<string> vValues) :
			m_pValues(new vector<string>(vValues))
		{
		}

		void SetCollNames(shared_ptr<vector<string> > pvNames)
		{
			m_pNames = pvNames;
			while(m_pValues->size() < m_pNames->size())
				m_pValues->push_back("");

			if (m_pNames->size())
				m_pValues->resize(m_pNames->size());
		}

		const size_t GetColsCount() const {return m_pValues->size();}

		string operator[] (size_t nCol)
		{
			assert(nCol < m_pValues->size());
			return (*m_pValues)[nCol];
		}
		const string operator[] (string strCol) const
		{
			string strRet;
			for (size_t n=0; n<m_pNames->size(); n++)
			{
				if ((*m_pNames)[n] != strCol)
					continue;

				assert(n < m_pValues->size());
				return (*m_pValues)[n];
			}
			//assert(false);
			return strRet;
		}
	};

	class CTable
	{
		static sqlite3 *gpDataBase;
		static bool gTransactionState;
		shared_ptr<vector<string> > m_pColNames;
		shared_ptr<vector<CRow> > m_pRows;
		string m_strTableName, m_strQuery;
	public:
		CTable(const string strTableName, const string strQuery = "") :
			m_strTableName(strTableName), m_strQuery(strQuery), m_pColNames(new vector<string>), m_pRows(new vector<CRow>)
		{
			CreateDatabase(DATABASE);
		}
		~CTable()
		{
		}

		static void BeginTransaction()
		{
			if (gTransactionState)
				return;

			while(!ExecuteSQL("BEGIN EXCLUSIVE TRANSACTION"))
				sqlite3_sleep(10);

			gTransactionState = true;
		}
		static void CommitTransaction()
		{
			if (!gTransactionState)
				return;

			while(!ExecuteSQL("COMMIT TRANSACTION"))
				sqlite3_sleep(10);

			gTransactionState = false;
		}

		const CTable InsertOrReplace(char const* fmt, ...) const;
		const CTable InsertOrAbort(char const* fmt, ...) const;
		//const CTable Count() const;

		const string ToJSON() const;

		static void CreateDatabase(const string strFilePath)
		{
			DEBUG_LOG("CreateDatabase start");
			utils::CMutex mutex;
			if (!gpDataBase)
			{
				char *szErrMsg = 0;
				int rc = sqlite3_open(strFilePath.c_str(), &gpDataBase);
				if (rc != SQLITE_OK)
				{
					sqlite3_free(szErrMsg);
					gpDataBase = NULL;
					DEBUG_LOG("CreateDatabase fail");
#ifdef _DEBUG
					__asm int 3;
#endif
				}

				gTransactionState = false;
			}
		}
		static bool ExecuteSQL(char const* fmt, ...)
		{
			if (!gpDataBase)
				return false;

			va_list ap;
			va_start(ap, fmt);
			const string strSQL = utils::dupvprintf(fmt, ap);
			va_end(ap);

			char *szErrMsg = 0;
			int rc = sqlite3_exec(gpDataBase, strSQL.c_str(), 0, 0, &szErrMsg);

			if( rc != SQLITE_OK )
			{
				sqlite3_free(szErrMsg);
				return false;
			}
			return true;
		}
		const string GetColName(size_t n) const
		{
			assert(n < m_pColNames->size());
			return (*m_pColNames)[n];
		}
		void InsertRow(CRow row) const;
		//void UpdateRow(const string strSet, const string strWhere) const;
		void AddRow(CRow row)
		{
			if (!m_pColNames->size())
				(*m_pColNames) = *row.m_pNames;

			row.SetCollNames(m_pColNames);
			m_pRows->push_back(row);
		}
		void AddRow(int nColumnCount, char **ppszColTextArray, char **azColName)
		{
			if (!m_pColNames->size())
			{
				for (int n=0; n<nColumnCount; n++)
					m_pColNames->push_back(azColName[n]);
			}

			m_pRows->push_back(CRow(nColumnCount, ppszColTextArray, *m_pColNames));
		}

		const size_t GetCollsCount() const {return m_pColNames->size();}
		const size_t GetRowsCount() const {return m_pRows->size();}

		const CRow FindFirstRow(string strCollName, string strCollValue) const
		{
			vector<string> v;
			CRow ret(v);
			for (size_t n=0; n<m_pRows->size(); n++)
			{
				if ((*m_pRows)[n][strCollName] == strCollValue)
					return (*m_pRows)[n];
			}
			return ret;
		}
		CRow &operator[] (size_t nRow)
		{
			assert(nRow < m_pRows->size());
			return (*m_pRows)[nRow];
		}
		const CRow operator[] (size_t nRow) const
		{
			assert(nRow < m_pRows->size());
			return (*m_pRows)[nRow];
		}

		const CTable GetAllTable(const string strSelect = "*") const;
		const CTable GetAllTableDistinct(const string &strDistinctColumns) const;
		const CTable Update() const;
		static const CTable CreateTable(const string &strTableName, const string &strQuery);
		const CTable Where(char const* fmt, ...) const;
		const CTable OrderBy(char const* fmt, ...) const;
		const CTable And(char const* fmt, ...) const;
		const CTable Insert(char const* fmt, ...) const;
		const CTable Set(char const* fmt, ...) const;
	};
}
#endif
