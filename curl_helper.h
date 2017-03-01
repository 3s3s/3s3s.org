#ifndef _curl_helper
#define _curl_helper
#include "log.h"
#include <vector>
#include <string.h>
#include <memory>
#include <map>

#define MYCURL

#ifdef MYCURL
#define CURL_STATICLIB

#include <curl/curl.h>

#include <sstream>

using namespace std;

#define MAX_EASY_HANDLES_COUNT	10000
namespace curl
{
	class CUrlHandle
	{
		struct curl_slist *m_pHeaders;
		CURL *m_pEasyHandle;
		bool m_bDone, m_bIsPost;
		vector<unsigned char> m_vReadedBytes, m_vWaitForWriteBytes;
		//size_t m_nWaitForWriteSize;

		time_t m_tmLastTime;

		explicit CUrlHandle(const CUrlHandle &handle) {}
	public:
		CUrlHandle(CURL *easy_handle) : m_bDone(false), m_bIsPost(false), m_pEasyHandle(easy_handle), m_pHeaders(NULL),
			/*m_nWaitForWriteSize(0),*/ m_tmLastTime(time(NULL))
		{}
		~CUrlHandle()
		{
			if (m_pHeaders)
				curl_slist_free_all(m_pHeaders);
#ifdef _DEBUG
			if (m_vReadedBytes.size())
				__asm nop;
#endif
		}
		CURL *GetHandle() const {return m_pEasyHandle;}
		vector<unsigned char> *GetReadedBytes() {return &m_vReadedBytes;}
		const vector<unsigned char> *GetWritedBytes() const {return &m_vWaitForWriteBytes;}
		inline const size_t GetInBufferSize() const {return m_vWaitForWriteBytes.size();/*m_nWaitForWriteSize;*/}
		inline void AddInBuffer(const vector<unsigned char> *pInBuffer, size_t nPos = -1)
		{
			if ((nPos == -1) || (nPos >= m_vWaitForWriteBytes.size()))
				nPos = m_vWaitForWriteBytes.size();

			if (nPos == 0 && (!pInBuffer->size()))
			{
				m_vWaitForWriteBytes.clear();
				//m_nWaitForWriteSize = 0;
				return;
			}

			m_tmLastTime = time(NULL);
			m_vWaitForWriteBytes.resize(nPos+pInBuffer->size());
			memcpy(&m_vWaitForWriteBytes[nPos], &pInBuffer->at(0), pInBuffer->size());

			//m_nWaitForWriteSize = m_vWaitForWriteBytes.size();
		}
		inline void MoveInBufferTo(void *ptr, size_t nStartFrom, size_t nCount)
		{
			if ((nStartFrom >= m_vWaitForWriteBytes.size()) || (nCount+nStartFrom > m_vWaitForWriteBytes.size()))
				return;

			memcpy(ptr, &m_vWaitForWriteBytes.at(nStartFrom), nCount);

			if (m_vWaitForWriteBytes.size() == nCount)
			{
				m_vWaitForWriteBytes.clear();
				//m_nWaitForWriteSize = 0;
				return;
			}

			m_tmLastTime = time(NULL);
			vector<unsigned char> tmp;
			tmp.resize(m_vWaitForWriteBytes.size() - nCount);
			
			memcpy(&tmp[0], &m_vWaitForWriteBytes.at(nCount), tmp.size());

			m_vWaitForWriteBytes = move(tmp);
			//m_nWaitForWriteSize = m_vWaitForWriteBytes.size();
		}
		inline void SetDone(bool bDone) {m_bDone = bDone;}
		inline const bool IsDone() const {return m_bDone;}

		inline const size_t GetBytesInMemory() const 
		{
			return m_vReadedBytes.size() + m_vWaitForWriteBytes.size();
		}
		inline void SetPostOption() 
		{
			m_bIsPost = true;
			curl_easy_setopt(m_pEasyHandle, CURLOPT_POST, true);
		}
		bool IsAllowContinue()
		{
			if (time(NULL) - m_tmLastTime > 60)
				SetDone(true);
			//if (m_bIsPost && (!m_vWaitForWriteBytes.size()))
			//	return false;
			return true;
		}
		void AddHeaders(const vector<string> &vHeaders)
		{
			if (!m_pEasyHandle)
				return;
			for (size_t n=0; n<vHeaders.size(); n++)
				m_pHeaders = curl_slist_append(m_pHeaders, vHeaders[n].c_str());  
			
			curl_easy_setopt (m_pEasyHandle, CURLOPT_HTTPHEADER, m_pHeaders);
		}
	};
	class CCurlHandles
	{
		CURLM *m_curlM;
		map<CURL *, shared_ptr<CUrlHandle> > m_CurlHandles;

		vector<shared_ptr<CUrlHandle> > m_vQueuedHandles;

		void ResetAllClients()
		{
			DEBUG_LOG("ResetAllClients m_CurlHandles.size() = %i", m_CurlHandles.size());
			/*for (auto it=m_CurlHandles.begin(); it != m_CurlHandles.end(); it++)
				it->second->SetDone(true);

			m_vQueuedHandles.clear();*/
			Cleanup();
		}

		bool IsAllowContinue()
		{
			size_t nBytesInMemory = 0;
			for (auto it=m_CurlHandles.begin(); it != m_CurlHandles.end(); it++)
			{
				nBytesInMemory += it->second->GetBytesInMemory();
				if (!it->second->IsAllowContinue())
					return false;
			}
//#ifndef _DEBUG
			if (nBytesInMemory > 100000000)
//#else
//			if (nBytesInMemory > 1000)
//#endif
			{
				ResetAllClients();
				return false;
			}
			
			return true;
		}
		explicit CCurlHandles(const CCurlHandles &handles) {}
	public:
		CCurlHandles()
		{
			curl_global_init(CURL_GLOBAL_DEFAULT);
			m_curlM = curl_multi_init();
		}
		~CCurlHandles()
		{
			Cleanup();

			curl_multi_cleanup(m_curlM);
			curl_global_cleanup();

			m_curlM = NULL;
		}
		void Continue()
		{
			if (!IsAllowContinue())
			{
				DEBUG_LOG("!AllowContinue");
				return;
			}

			int still_running;
			CURLMcode code = curl_multi_perform(m_curlM, &still_running);

			int msgs_in_queue = 1;
			while(msgs_in_queue)
			{
				CURLMsg *msg = curl_multi_info_read(m_curlM, &msgs_in_queue);
				if (!msg)
					break;

				if (msg->msg != CURLMSG_DONE)
					continue;

				Get(msg->easy_handle)->SetDone(true);
			}
		}
		inline void Add(CURL *easy_handle) 
		{
			if (m_CurlHandles.size() > MAX_EASY_HANDLES_COUNT)
			{
				m_vQueuedHandles.push_back(shared_ptr<CUrlHandle>(new CUrlHandle(easy_handle)));
				return;
			}

			curl_multi_add_handle(m_curlM, easy_handle);
			m_CurlHandles[easy_handle] = shared_ptr<CUrlHandle>(new CUrlHandle(easy_handle));
		}
		const uint32_t GetQueuedCount() const {return m_vQueuedHandles.size();}
		inline CURL* UpdateQueue()
		{
			if ((m_CurlHandles.size() >= MAX_EASY_HANDLES_COUNT) || (!m_vQueuedHandles.size()))
				return NULL;

			DEBUG_LOG("CURL UpdateQueue");
			shared_ptr<CUrlHandle> pHandle = m_vQueuedHandles[0];
			Add(pHandle->GetHandle());
			if (m_vQueuedHandles.size() == 1)
			{
				m_vQueuedHandles.clear();
				return pHandle->GetHandle();
			}
			
			vector<shared_ptr<CUrlHandle> > tmp;
			for (size_t n=1; n<m_vQueuedHandles.size(); n++)
			{
				if (m_vQueuedHandles[n]->GetHandle() == pHandle->GetHandle())
					continue;

				tmp.push_back(m_vQueuedHandles[n]);
			}
			m_vQueuedHandles = move(tmp);

			return pHandle->GetHandle();
		}
		inline void DeleteQueuedHandle(CURL *easy_handle)
		{
			DEBUG_LOG("DeleteQueuedHandle start");
			curl_easy_cleanup(easy_handle);

			vector<shared_ptr<CUrlHandle> > tmp;
			for (size_t n=0; n<m_vQueuedHandles.size(); n++)
			{
				if (m_vQueuedHandles[n]->GetHandle() == easy_handle)
					continue;

				tmp.push_back(m_vQueuedHandles[n]);
			}
			m_vQueuedHandles = move(tmp);
		}
		const bool Find(CURL *easy_handle) const
		{
			if (!easy_handle)
				return false;

			if (m_CurlHandles.find(easy_handle) != m_CurlHandles.end())
				return true;

			for (size_t n=0; n<m_vQueuedHandles.size(); n++)
			{
				if (m_vQueuedHandles[n]->GetHandle() == easy_handle)
					return true;
			}
				
			return false;
		}
		inline void Delete(CURL *easy_handle)
		{
			if (!easy_handle || Get(easy_handle)->GetHandle() != easy_handle)
				return;

			auto it = m_CurlHandles.find(easy_handle);
			if (it != m_CurlHandles.end())
			{
				m_CurlHandles.erase(easy_handle);
		
				if (m_curlM)
				{
					DEBUG_LOG("curl_multi_remove_handle start");
					curl_multi_remove_handle(m_curlM, easy_handle);
				}
			}

//			if (it == m_CurlHandles.end())
			
			DeleteQueuedHandle(easy_handle);
		}
		shared_ptr<CUrlHandle> Get(CURL *easy_handle)
		{
			//DEBUG_LOG("Get handle easy_handle");
			auto it = m_CurlHandles.find(easy_handle);
			if (it != m_CurlHandles.end())
			{
//				DEBUG_LOG("return mapped handle");
				return it->second;
			}

			for (size_t n=0; n<m_vQueuedHandles.size(); n++)
			{
				if (m_vQueuedHandles[n]->GetHandle() == easy_handle)
				{
					DEBUG_LOG("return queued handle");
					return m_vQueuedHandles[n];
				}
			}
			//DEBUG_LOG("return null eazy_handle");
			return shared_ptr<CUrlHandle>(new CUrlHandle(NULL));
		}
		void Cleanup()
		{
			//for (auto it=m_CurlHandles.begin(); it != m_CurlHandles.end(); it++)
			auto it = m_CurlHandles.begin();
			while( it != m_CurlHandles.end())
			{
				DEBUG_LOG("Delete handle with BytesCount=%i; readedbytes=%i, writedbytes=%i", it->second->GetBytesInMemory(), it->second->GetReadedBytes()->size(), it->second->GetWritedBytes()->size());
				Delete(it->first);

				it = m_CurlHandles.begin();
			}

			m_CurlHandles.clear();
		}
		inline const size_t Count() const {return m_CurlHandles.size();}
	};
	class CUrl
	{
		static CCurlHandles m_Handles;//map<CURL *, shared_ptr<CUrlHandle> > m_mapHandles;
		explicit CUrl(const CUrl &curl) {}
	public:
		CUrl() {}
		static size_t ReadCallback(void *ptr, size_t size, size_t nmemb, void *userdata)
		{
			const size_t nInBufferSize = m_Handles.Get((CURL *)userdata)->GetInBufferSize();

			DEBUG_LOG("CUrl::ReadCallback sizeForAdd=%i; AllSize=%i", size*nmemb, nInBufferSize);
			
			size_t nWriteSize = 0;

			if (size*nmemb && nInBufferSize)
			{
				nWriteSize = nInBufferSize;
				if (nWriteSize > size*nmemb)
					nWriteSize = size*nmemb;
				//if (nWriteSize > 100000)
				//	nWriteSize = 100000;

				m_Handles.Get((CURL *)userdata)->MoveInBufferTo(ptr, 0, nWriteSize);

				if (size*nmemb > 1000000)
				{
					Pause(userdata, true);
					return CURL_READFUNC_PAUSE;
				}

			}
			DEBUG_LOG("CUrl::ReadCallback end");
			return nWriteSize;
		}
		static size_t DownloadCallback(char *ptr, size_t size, size_t nmemb, void *userdata)
		{
			DEBUG_LOG("CURL DownloadCallback size=%i", size*nmemb);

			vector<unsigned char> *pvReadBytes = GetReadedBytes((CURL *)userdata);

			/*if (pvReadBytes->size() > 1000000)
			{
				DEBUG_LOG("ERROR: too large buffer in curl DownloadCallback (curl will by paused)");
				Pause(userdata, true);
				//return 0;
			}*/

			if (pvReadBytes->size() > 10000000)
			{
				DEBUG_LOG("ERROR: TOOOOO large buffer in curl DownloadCallback (curl will by stopped!!!)");
				return 0;
			}

			if (size*nmemb)
			{
				size_t nOldSize = pvReadBytes->size();
				pvReadBytes->resize(nOldSize + size*nmemb);

				memcpy(&pvReadBytes->at(nOldSize), ptr, size*nmemb);
				
				if (size*nmemb > 1000000 || pvReadBytes->size() > 1000000)
				{
					Pause(userdata, true);
					return CURL_WRITEFUNC_PAUSE;
				}

				return size*nmemb;
			}

			return 0;

		}
		static string URLDecode(string str)
		{
			int nOut;
			char *pszDecodded = curl_easy_unescape(NULL, str.c_str() , str.length(), &nOut );
			if (!pszDecodded)
				return str;

			string strRet = pszDecodded;
			curl_free(pszDecodded);

			return strRet;

		}
		static string URLEncode(string str)
		{
			//CURL *http_handle = curl_easy_init();
			//if (!http_handle)
			//	return str;

			char *pszEncodded = curl_easy_escape(NULL, str.c_str() , str.length() );
			if (!pszEncodded)
				return str;

			string strRet = pszEncodded;
			curl_free(pszEncodded);

			//curl_easy_cleanup(http_handle);
			return strRet;
		}
		inline static vector<unsigned char> *GetReadedBytes(CURL *easy_handle) {return m_Handles.Get(easy_handle)->GetReadedBytes();}
		inline static const vector<unsigned char> *GetWritedBytes(CURL *easy_handle) {return m_Handles.Get(easy_handle)->GetWritedBytes();}
		void SetOptPost(CURL *easy_handle, vector<unsigned char> *pInBuffer)
		{
			if (!easy_handle)
				return;

			m_Handles.Get(easy_handle)->SetPostOption();

			if (pInBuffer->size())
				m_Handles.Get(easy_handle)->AddInBuffer(pInBuffer);
		}
		void AddWritedBytes(CURL *easy_handle, vector<unsigned char> *pInBuffer)
		{
			if (!easy_handle)
				return;
			
#ifdef _DEBUG
			DEBUG_LOG("CUrl::AddWritedBytes size=%i", pInBuffer->size());
#endif
			if (pInBuffer->size())
				m_Handles.Get(easy_handle)->AddInBuffer(pInBuffer);
		}
		void AddHandle(CURL *easy_handle)
		{
			m_Handles.Add(easy_handle);

			DEBUG_LOG("m_vEasyHandles.size=%i; queued=%i", m_Handles.Count(), m_Handles.GetQueuedCount());
		}
		void AddHeaders(CURL *easy_handle, const vector<string> &vHeaders)
		{
			m_Handles.Get(easy_handle)->AddHeaders(vHeaders);
		}
		void DeleteHandle(CURL *easy_handle)
		{
			if (!easy_handle)
				return;

			m_Handles.Delete(easy_handle);
			
			m_Handles.UpdateQueue();
		}
		const bool HaveHandle(CURL *easy_handle) const {return m_Handles.Find(easy_handle);}
		inline const bool IsDone(CURL *easy_handle) const {return m_Handles.Get(easy_handle)->IsDone();}
		static void Pause(CURL *easy_handle, bool bPause = true) { curl_easy_pause(easy_handle, bPause ? CURLPAUSE_ALL : CURLPAUSE_CONT); }
		void Continue()
		{
			CURL *pAdded = m_Handles.UpdateQueue();

			m_Handles.Continue();
		}
		static int Log(CURL *handle, curl_infotype type, char *data, size_t size, void *userp)
		{
			switch (type)
			{
				case CURLINFO_TEXT:
				case CURLINFO_HEADER_IN:
				case CURLINFO_HEADER_OUT:
					if (data && size)
					{
						vector<unsigned char> temp(size+1);
						memcpy(&temp[0], data, size);
						temp[size] = 0;

						DEBUG_LOG("%s", (const char *)&temp[0]);
					}
			}
			return 0;
		}
	};
}

#endif

#endif //_curl_helper