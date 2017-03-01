#ifndef _HTML_FRAMEWORK
#define _HTML_FRAMEWORK
#ifdef WIN32
#include "zlib-1.2.7/zlib.h"
#else
#include <zlib.h>
#endif
#include <memory>
#include <vector>
#include <sstream>
#include <string>
#include <assert.h>
#include "md5/md5.h"
#include <locale>
#include <iostream>
#include <algorithm>

using namespace std;

namespace html
{
	class utils
	{
	public:
		static void Replace(string& str, const int nPos, const string& str1, const string& str2)
		{
			const auto str2Size(str2.size());
			const auto str1Size(str1.size());

			auto n = nPos;
			while (string::npos != (n = str.find(str1, n))) {
				str.replace(n, str1Size, str2);
				n += str2Size;
			}
		}
		static vector<string> GetTagsArray(const string strContent, const string strTagName)
		{
			vector<string> vRet;

			string strPart = strContent;
			while(1)
			{
				const int nPosStartBegin = ci_find_substr(strPart, "<"+strTagName);
				if (nPosStartBegin == -1)
					break;

				int nPosStartEnd = ci_find_substr(strPart, ">", nPosStartBegin);
				if ((nPosStartEnd == -1) || nPosStartEnd <= nPosStartBegin)
					break;

				const string strBegin = strPart.substr(nPosStartBegin, nPosStartEnd-nPosStartBegin+1);

				if (strTagName == "meta")
				{
					vRet.push_back(strPart.substr(nPosStartBegin, nPosStartEnd-nPosStartBegin+1));
					
					const string strTemp = strPart.substr(nPosStartEnd+1);
					strPart = strTemp;
				}
#if 0
				int nEndEnd = -1;
				int nEndStart1 = ci_find_substr(strPart, "</"+strTagName, nPosStartBegin);
				int nEndStart2 = ci_find_substr(strPart, "/>", nPosStartBegin);
				if (nEndStart1 == -1 && nEndStart2 == -1 && (strTagName != "meta"))
				{
					vRet.push_back(strBegin);
					
					const string strTemp = strPart.substr(nPosStartEnd);
					strPart = strTemp;
					break;
				}
				if (nEndStart2 < nEndStart1 && (nEndStart2 != -1) && (strTagName == "meta"))
					nEndEnd = nEndStart2+1;
				else
					nEndEnd = strPart.find(">", nEndStart1+1);

				if ((nEndEnd == -1) || (nEndEnd <= nPosStartBegin))
					break;
				vRet.push_back(strPart.substr(nPosStartBegin, nEndEnd-nPosStartBegin));
				
				const string strTemp = strPart.substr(nEndEnd);
				strPart = strTemp;
#endif
			}

			return vRet;
		}
		static string md5(string strFrom)
		{
			MD5_CTX mdContext;
			unsigned int len = strFrom.length();

			MD5Init (&mdContext);
			MD5Update (&mdContext, (unsigned char *)strFrom.c_str(), len);
			MD5Final (&mdContext);

			string strRet;
		    for (int i = 0; i < 16; i++)
			{
				char sz[3];
				sprintf (sz, "%02x", mdContext.digest[i]);
				
				strRet += sz;
			}
			return strRet;
		}
		static const string GetTagContent(const string strAllContent, const string strTagName)
		{
			const int nPosStartBegin = ci_find_substr(strAllContent, "<"+strTagName);
			if (nPosStartBegin == -1)
				return "";

			const int nPosStartEnd = ci_find_substr(strAllContent, ">", nPosStartBegin);
			if (nPosStartEnd == -1)
				return "";

			const int nPosEndBegin = ci_find_substr(strAllContent, "</"+strTagName, nPosStartEnd);
			if (nPosEndBegin == -1)
				return "";

			return strAllContent.substr(nPosStartEnd+1, nPosEndBegin-nPosStartEnd-1);
		}
		static void compress_memory(void *in_data, const size_t in_data_size, vector<unsigned char> &out_data)
		{
			vector<unsigned char> buffer;

			const size_t BUFSIZE = 128 * 1024;
			unsigned char temp_buffer[BUFSIZE];

			z_stream strm;
			strm.zalloc = 0;
			strm.zfree = 0;
			strm.next_in = reinterpret_cast<unsigned char *>(in_data);
			strm.avail_in = in_data_size;
			strm.next_out = temp_buffer;
			strm.avail_out = BUFSIZE;
			deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, (15+16), 8, Z_DEFAULT_STRATEGY);

			while (strm.avail_in != 0)
			{
				int res = deflate(&strm, Z_NO_FLUSH);
				assert(res == Z_OK);
				if (strm.avail_out == 0)
				{
					buffer.insert(buffer.end(), temp_buffer, temp_buffer + BUFSIZE);
					strm.next_out = temp_buffer;
					strm.avail_out = BUFSIZE;
				}
			}

			int deflate_res = Z_OK;
			while (deflate_res == Z_OK)
			{
				if (strm.avail_out == 0)
				{
					buffer.insert(buffer.end(), temp_buffer, temp_buffer + BUFSIZE);
					strm.next_out = temp_buffer;
					strm.avail_out = BUFSIZE;
				}
				deflate_res = deflate(&strm, Z_FINISH);
			}

			assert(deflate_res == Z_STREAM_END);
			buffer.insert(buffer.end(), temp_buffer, temp_buffer + BUFSIZE - strm.avail_out);
			deflateEnd(&strm);

			out_data.swap(buffer);
		}		
		static bool gzipInflate( const vector<unsigned char>& compressedBytes, vector<unsigned char>& uncompressedBytes ) {  
		  if ( compressedBytes.size() == 0 ) {  
			uncompressedBytes = compressedBytes ;  
			return true ;  
		  }  
  
		  uncompressedBytes.clear() ;  
  
		  unsigned full_length = compressedBytes.size() ;  
		  unsigned half_length = compressedBytes.size() / 2;  
  
		  unsigned uncompLength = full_length ;  
		  char* uncomp = (char*) calloc( sizeof(char), uncompLength );  
  
		  z_stream strm;  
		  strm.next_in = (Bytef *) &compressedBytes[0];  
		  strm.avail_in = compressedBytes.size() ;  
		  strm.total_out = 0;  
		  strm.zalloc = Z_NULL;  
		  strm.zfree = Z_NULL;  
  
		  bool done = false ;  
  
		  if (inflateInit2(&strm, (16+MAX_WBITS)) != Z_OK) {  
			free( uncomp );  
			return false;  
		  }  
  
		  while (!done) {  
			// If our output buffer is too small  
			if (strm.total_out >= uncompLength ) {  
			  // Increase size of output buffer  
			  char* uncomp2 = (char*) calloc( sizeof(char), uncompLength + half_length );  
			  memcpy( uncomp2, uncomp, uncompLength );  
			  uncompLength += half_length ;  
			  free( uncomp );  
			  uncomp = uncomp2 ;  
			}  
  
			strm.next_out = (Bytef *) (uncomp + strm.total_out);  
			strm.avail_out = uncompLength - strm.total_out;  
  
			// Inflate another chunk.  
			int err = inflate (&strm, Z_SYNC_FLUSH);  
			if (err == Z_STREAM_END) done = true;  
			else if (err != Z_OK)  {  
			  break;  
			}  
		  }  
  
		  if (inflateEnd (&strm) != Z_OK) {  
			free( uncomp );  
			return false;  
		  }  
  
		 /* for ( size_t i=0; i<strm.total_out; ++i ) {  
			uncompressedBytes += uncomp[ i ];  
		  }  */
		  uncompressedBytes.resize(strm.total_out);
		  memcpy(&uncompressedBytes[0], uncomp, strm.total_out);

		  free( uncomp );  
		  return true ;  
		}  
		static char easytolower(char in)
		{
		  if(in<='Z' && in>='A')
			return in-('Z'-'z');
		  if(in<='ß' && in>='À')
			return in-('ß'-'ÿ');
		  return in;
		}

		static void ascii2utf8_static(const char *pszASCII, string &strOut)
		{
			string strIn = pszASCII;

			static const short utf[256] =
			{
				0,1,2,3,4,5,6,7,8,9,0xa,0xb,0xc,0xd,0xe,0xf,0x10,0x11,0x12,0x13,0x14,0x15,0x16,
				0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23,0x24,0x25,0x26,
				0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,0x30,0x31,0x32,0x33,0x34,0x35,0x36,
				0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,0x40,0x41,0x42,0x43,0x44,0x45,0x46,
				0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,0x50,0x51,0x52,0x53,0x54,0x55,0x56,
				0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,0x60,0x61,0x62,0x63,0x64,0x65,0x66,
				0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,0x70,0x71,0x72,0x73,0x74,0x75,0x76,
				0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,0x402,0x403,0x201a,0x453,0x201e,
				0x2026,0x2020,0x2021,0x20ac,0x2030,0x409,0x2039,0x40a,0x40c,0x40b,0x40f,0x452,
				0x2018,0x2019,0x201c,0x201d,0x2022,0x2013,0x2014,0,0x2122,0x459,0x203a,0x45a,
				0x45c,0x45b,0x45f,0xa0,0x40e,0x45e,0x408,0xa4,0x490,0xa6,0xa7,0x401,0xa9,0x404,
				0xab,0xac,0xad,0xae,0x407,0xb0,0xb1,0x406,0x456,0x491,0xb5,0xb6,0xb7,0x451,
				0x2116,0x454,0xbb,0x458,0x405,0x455,0x457,0x410,0x411,0x412,0x413,0x414,0x415,
				0x416,0x417,0x418,0x419,0x41a,0x41b,0x41c,0x41d,0x41e,0x41f,0x420,0x421,0x422,
				0x423,0x424,0x425,0x426,0x427,0x428,0x429,0x42a,0x42b,0x42c,0x42d,0x42e,0x42f,
				0x430,0x431,0x432,0x433,0x434,0x435,0x436,0x437,0x438,0x439,0x43a,0x43b,0x43c,
				0x43d,0x43e,0x43f,0x440,0x441,0x442,0x443,0x444,0x445,0x446,0x447,0x448,0x449,
				0x44a,0x44b,0x44c,0x44d,0x44e,0x44f
			};

			int cnt = strIn.length(),
			i = 0, j = 0;

			for(; i < cnt; ++i)
			{
				long c = utf[(unsigned char)strIn[i]];

				if (c < 0x80) strOut += c;
				else if (c < 0x800)
				{
					strOut += c >> 6 | 0xc0;
					strOut += c & 0x3f | 0x80;
				}
				else if (c < 0x10000)
				{
					strOut += c >> 12 | 0xe0;
					strOut += c >> 6 & 0x3f | 0x80;
					strOut += c & 0x3f | 0x80;
				}
				else if (c < 0x200000)
				{
					strOut += c >> 18 | 0xf0;
					strOut += c >> 12 & 0x3f | 0x80;
					strOut += c >> 6 & 0x3f | 0x80;
					strOut += c & 0x3f | 0x80;
				}
			}
		}
		static const string ConvertString (const char *pszASCII)
		{
			string strRet;
			ascii2utf8_static(pszASCII, strRet);
			return strRet;
		}
		template<typename charT>
		struct my_equal {
			my_equal( const std::locale& loc ) : loc_(loc) {}
			bool operator()(charT ch1, charT ch2) {
				return std::toupper(ch1, loc_) == std::toupper(ch2, loc_);
			}
		private:
			const std::locale& loc_;
		};

		// find substring (case insensitive)
		template<typename T>
		static int ci_find_substr0( const T& strSrc, const T& strSubstr, const std::locale& loc = std::locale() )
		{
			auto it = std::search( strSrc.begin(), strSrc.end(), 
				strSubstr.begin(), strSubstr.end(), my_equal<const char>(loc) );
			if ( it != strSrc.end() ) return it - strSrc.begin();
			else return -1; // not found
		}
		static int ci_find_substr( const string& strSource, const string strSubstring, const int nPosStart = 0)
		{
			return ci_find_substr0(strSource.substr(nPosStart), strSubstring)+nPosStart;
		}
	};
#define ATTRIBUTE(name) const CBaseHtmlElement name(const string strName) const {return AddAttribute(#name, strName);}
	class CBaseHtmlElement
	{
		string m_strName, m_strClass, m_strInnerHTML, m_strAttributes;

	protected:
		CBaseHtmlElement(const string strName, const string strAttributes = "", const string strClass = "", const string strInnerHTML = "") :
			m_strName(strName), m_strClass(strClass), m_strInnerHTML(strInnerHTML), m_strAttributes(strAttributes)
		{
			if (m_strAttributes == " id=''") m_strAttributes = "";
		}

		const CBaseHtmlElement AddAttribute(const string strName, const string strValue) const
		{
			return CBaseHtmlElement(m_strName, m_strAttributes+" "+strName+"=\""+strValue+"\"", m_strClass, m_strInnerHTML);
		}
	public:
		const string innerHTML() const {return m_strInnerHTML;}

		const CBaseHtmlElement AddClass(const string strValue) const
		{
			return CBaseHtmlElement(m_strName, m_strAttributes, m_strClass + " " + strValue, m_strInnerHTML);
		}
		const CBaseHtmlElement AddAttribute(const string strAttribute) const
		{
			return CBaseHtmlElement(m_strName, m_strAttributes+" "+strAttribute, m_strClass, m_strInnerHTML);
		}
		const string outerHTML() const
		{
			ostringstream strContent, strClass, strAttributes;
			
			if (m_strClass.length())
				strClass << " class=\"" << m_strClass << "\"";

			if (m_strAttributes.length())
				strAttributes << m_strAttributes;

			strContent << 
				"<" << m_strName << strClass.str() << strAttributes.str();
			
			if ((m_strName == "meta") || (m_strName == "link"))
				strContent << "/>\r\n";
			else if (m_strName == "input")
				strContent << ">\r\n";
			else
				strContent << ">\r\n" << m_strInnerHTML << "</" << m_strName << ">\r\n";

			return strContent.str();
		}
		const CBaseHtmlElement add(const CBaseHtmlElement &element) const
		{
			return CBaseHtmlElement(m_strName, m_strAttributes, m_strClass, m_strInnerHTML+element.outerHTML());
		}
		const CBaseHtmlElement operator<<(const CBaseHtmlElement &element) const {return add(element);}
		const CBaseHtmlElement operator<<(const string &strHTML) const {return add(strHTML);}

		const CBaseHtmlElement add(const string strText) const
		{
			return CBaseHtmlElement(m_strName, m_strAttributes, m_strClass, m_strInnerHTML+strText);
		}

		ATTRIBUTE(style);
		ATTRIBUTE(title);
		ATTRIBUTE(src);
		ATTRIBUTE(href);
		ATTRIBUTE(onclick);
		ATTRIBUTE(rel);
		ATTRIBUTE(type);
		ATTRIBUTE(http_equiv);
		ATTRIBUTE(content);
		ATTRIBUTE(name);
		ATTRIBUTE(prop);
		ATTRIBUTE(id);
		ATTRIBUTE(size);
		ATTRIBUTE(maxlength);
	};

#define DECLARE_TAG(name) \
	class name : public CBaseHtmlElement \
	{ \
	public: \
		name(const string strClass = "", const string strID = "") : CBaseHtmlElement(#name, " id='"+strID+"'", strClass, "") {} \
	}

	DECLARE_TAG(title);
	DECLARE_TAG(link);
	DECLARE_TAG(meta);
	DECLARE_TAG(script);
	DECLARE_TAG(table);
	DECLARE_TAG(tr);
	DECLARE_TAG(td);
	DECLARE_TAG(h1);
	DECLARE_TAG(h2);
	DECLARE_TAG(h3);
	DECLARE_TAG(head);
	DECLARE_TAG(body);
	DECLARE_TAG(ul);
	DECLARE_TAG(li);
	DECLARE_TAG(span);
	DECLARE_TAG(input);
	DECLARE_TAG(center);
	DECLARE_TAG(button);
	DECLARE_TAG(DIV);
	DECLARE_TAG(A);
	DECLARE_TAG(B);
	DECLARE_TAG(P);
	DECLARE_TAG(HTML);
	DECLARE_TAG(textarea);
	DECLARE_TAG(form);
	DECLARE_TAG(tt);
	DECLARE_TAG(IMG);

	template<class T>
	class CElement
	{
		shared_ptr<CBaseHtmlElement> m_pElement;
	public:		
		CElement() : m_pElement(new T) {}
		const string outerHTML() const
		{
			return m_pElement->outerHTML();
		}
		void insert(const CBaseHtmlElement &element)
		{
			m_pElement = shared_ptr<CBaseHtmlElement>(new CBaseHtmlElement(m_pElement->add(element)));
		}
		void insert(const string strHTML)
		{
			m_pElement = shared_ptr<CBaseHtmlElement>(new CBaseHtmlElement(m_pElement->add(strHTML)));
		}
		void AddCSS(string strCSS) 
		{
			m_pElement = shared_ptr<CBaseHtmlElement>(new CBaseHtmlElement(m_pElement->AddClass(strCSS)));
		}
		void AddAttribute(string strAttribute)
		{
			m_pElement = shared_ptr<CBaseHtmlElement>(new CBaseHtmlElement(m_pElement->AddAttribute(strAttribute)));
		}

		operator string() const {return outerHTML();}
	};

	class CTableRow : public CElement<tr>
	{
		vector<CElement<td> > m_Cols;
	public:
		CTableRow(const size_t nCols)
		{
			for (size_t n=0; n<nCols; n++)
				m_Cols.push_back(CElement<td>());
		}
		const string outerHTML() const
		{
			CTableRow ret(*this);

			for (size_t n=0; n<m_Cols.size(); n++)
				ret.insert(m_Cols[n].outerHTML());
			
			return ((CElement<tr>)ret).outerHTML();
		}

		CElement<td> &operator[] (size_t nColl) {return m_Cols[nColl];}
	};
	class CTable : public CElement<table>
	{
		shared_ptr<vector<CTableRow> > m_pRows;
		size_t m_nColsCount;
	public:
		CTable(const size_t nRows, const size_t nCols, const string strClass = "", const string strID = "") :
			m_nColsCount(nCols), m_pRows(new vector<CTableRow>)
		{
			for (size_t n=0; n<nRows; n++)
				m_pRows->push_back(CTableRow(nCols));

			if (strClass.length())
				AddCSS(strClass);
			if (strID.length())
				AddAttribute("id='"+strID+"'");
		}

		const string outerHTML() const
		{
			CTable ret(*this);
			
			for (size_t n=0; n<m_pRows->size(); n++)
				ret.insert((*m_pRows)[n].outerHTML());

			return ((CElement<table>)ret).outerHTML();
		}

		CTableRow &operator[] (size_t nRow) {return (*m_pRows)[nRow];}
		operator string () const {return outerHTML();}

	};
}
#endif