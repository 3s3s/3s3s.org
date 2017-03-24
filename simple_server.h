#ifndef _SIMPLE_SERVER
#define _SIMPLE_SERVER

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "log.h"
#include <unordered_map>

//#include "StartupInfo.h"

//#include "custom_plugins.h"
#ifdef WIN32
#include <direct.h>
#include <sys/utime.h>
#include "./zlib-1.2.7/zlib.h"
#define _mkdir(a, b)	_mkdir(a)

#define utimbuf	_utimbuf
#define utime _utime

#else
#define _mkdir	mkdir
#define _fileno	fileno
#include <utime.h>
#include <zlib.h>
#endif

//#define _DEBUG
#define d2_printf	//
#ifdef __linux__
#define _EPOLL
#define _SEND_FILE
#define Sleep(a) usleep(a*1000)
#endif

#ifdef _DEBUG
#define d_printf	printf
#else
#define d_printf	//
#endif

#ifdef _WIN32
#define	_WIN32_WINNT 0x600
#include <io.h>

#	include <Winsock2.h>
#	include <WS2TCPIP.H>
#	pragma comment(lib, "ws2_32.lib")

#define SET_NONBLOCK(socket)	\
	if (true)					\
	{							\
		DWORD dw = true;			\
		ioctlsocket(socket, FIONBIO, &dw);	\
	}

#define SET_BLOCK(socket)		\
	if (true)					\
	{							\
		DWORD dw = false;			\
		ioctlsocket(socket, FIONBIO, &dw);	\
	}

#define BEGIN_SEND //
#define END_SEND  //

#define snprintf _snprintf
#define open _open
#define popen _popen
#define pclose _pclose
#define close _close
#define off_t _off_t

enum EPOLL_EVENTS
  {
    EPOLLIN = 0x001,
#define EPOLLIN EPOLLIN
    EPOLLPRI = 0x002,
#define EPOLLPRI EPOLLPRI
    EPOLLOUT = 0x004,
#define EPOLLOUT EPOLLOUT
    EPOLLRDNORM = 0x040,
#define EPOLLRDNORM EPOLLRDNORM
    EPOLLRDBAND = 0x080,
#define EPOLLRDBAND EPOLLRDBAND
    EPOLLWRNORM = 0x100,
#define EPOLLWRNORM EPOLLWRNORM
    EPOLLWRBAND = 0x200,
#define EPOLLWRBAND EPOLLWRBAND
    EPOLLMSG = 0x400,
#define EPOLLMSG EPOLLMSG
    EPOLLERR = 0x008,
#define EPOLLERR EPOLLERR
    EPOLLHUP = 0x010,
#define EPOLLHUP EPOLLHUP
    EPOLLRDHUP = 0x2000,
#define EPOLLRDHUP EPOLLRDHUP
    EPOLLONESHOT = (1 << 30),
#define EPOLLONESHOT EPOLLONESHOT
    EPOLLET = (1 << 31)
#define EPOLLET EPOLLET
  };


/* Valid opcodes ( "op" parameter ) to issue to epoll_ctl().  */
#define EPOLL_CTL_ADD 1      /* Add a file descriptor to the interface.  */
#define EPOLL_CTL_DEL 2      /* Remove a file descriptor from the interface.  */
#define EPOLL_CTL_MOD 3      /* Change file descriptor epoll_event structure.  */

typedef union epoll_data
{
    void				*ptr;
    int					fd;
    unsigned int		u32;
    unsigned __int64    u64;
} epoll_data_t;

struct epoll_event
{
    unsigned __int64     events;      /* Epoll events */
    epoll_data_t		 data;        /* User data variable */
};

#else
#	include <signal.h>
#	include <sys/socket.h>
#	include <sys/stat.h>
#	include <unistd.h>
#	include <ctype.h>
#	include <netinet/in.h>
#	include <netdb.h>
#	include <arpa/inet.h>
#	include <errno.h>

#define lseek64		_lseeki64
#define fsync(fd)	_flushall()
#define closesocket(socket)	close(socket)
#define WSAGetLastError()	errno

#define SET_NONBLOCK(socket)	\
	if (fcntl( socket, F_SETFL, fcntl( socket, F_GETFL, 0 ) | O_NONBLOCK ) < 0)	\
		d_printf("error in fcntl errno=%i\n", errno);

	//Ignoring 'broken pipe' signal
#define BEGIN_SEND					\
	struct sigaction sa;			\
	memset(&sa, 0, sizeof(sa));		\
	sa.sa_handler = SIG_IGN;		\
	sigaction(SIGPIPE, &sa, NULL);

	//Wait for 'broken pipe' signal
#define END_SEND					\
	memset(&sa, 0, sizeof(sa));		\
	sa.sa_handler = SIG_DFL;		\
	sigaction(SIGPIPE, &sa, NULL);

typedef unsigned int SOCKET;
typedef unsigned char BYTE;
#define S_OK			0
#define SD_SEND         0x01
#define SD_BOTH         0x02
#define MAX_PATH	255

#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR            (-1)
#define WSAEINPROGRESS			EINPROGRESS
#define WSAEWOULDBLOCK			EWOULDBLOCK

#endif

#ifdef __linux__
#include <sys/sendfile.h>
#include <sys/epoll.h>
#include <netinet/tcp.h>
#include <string.h>
#endif
#ifdef _WIN32
#define O_NONBLOCK	0
#endif
#include <assert.h>
#include <memory>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <iostream>
#include <sstream>
#include "curl_helper.h"
#include "html_framework.h"

#include <openssl/rsa.h>       /* SSLeay stuff */
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

using namespace std;
#ifdef MYCURL
using namespace curl;

//CUrl gCURL;
#endif

extern set<string> g_vWhiteListedIP;

namespace simple_server
{
	namespace utils
	{
		template<class T> inline void Replace(T& str, const int nPos, const T& str1, const T& str2)
		{
			const auto str2Size(str2.size());
			const auto str1Size(str1.size());

			auto n = nPos;
			while (T::npos != (n = str.find(str1, n))) {
				str.replace(n, str1Size, str2);
				n += str2Size;
			}
		}
	}
	enum ACTION
	{
		A_NULL,
		A_ACCEPTED,
		A_READING,
		A_HEADER_READED,
		A_SENDING,
		A_ALL_SENDED,
		A_ERROR,
		A_CGI_START,
		A_CGI_CONTINUE,
		A_CGI_END
	};

#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#  include <fcntl.h>
#  include <io.h>
#  define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#  define SET_BINARY_MODE(file)
#endif

#define CHUNK 16384

	class CFile
	{
		string m_strFileName;
		bool m_bGzip;

		explicit CFile(const CFile &file);
	public:
/* Compress from file source to file dest until EOF on source.
   def() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_STREAM_ERROR if an invalid compression
   level is supplied, Z_VERSION_ERROR if the version of zlib.h and the
   version of the library linked do not match, or Z_ERRNO if there is
   an error reading or writing the files. */
	static int def(FILE *source, FILE *dest)
	{
		int ret, flush;
		unsigned have;
		z_stream strm;
		unsigned char in[CHUNK];
		unsigned char out[CHUNK];

		/* allocate deflate state */
		strm.zalloc = Z_NULL;
		strm.zfree = Z_NULL;
		strm.opaque = Z_NULL;
		//ret = deflateInit(&strm, level);
		ret = deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, (15+16), 8, Z_DEFAULT_STRATEGY);
		if (ret != Z_OK)
			return ret;

		/* compress until end of file */
		do {
			strm.avail_in = fread(in, 1, CHUNK, source);
			if (ferror(source)) {
				(void)deflateEnd(&strm);
				return Z_ERRNO;
			}
			flush = feof(source) ? Z_FINISH : Z_NO_FLUSH;
			strm.next_in = in;

			/* run deflate() on input until output buffer not full, finish
			   compression if all of source has been read in */
			do {
				strm.avail_out = CHUNK;
				strm.next_out = out;
				ret = deflate(&strm, flush);    /* no bad return value */
				assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
				have = CHUNK - strm.avail_out;
				if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
					(void)deflateEnd(&strm);
					return Z_ERRNO;
				}
			} while (strm.avail_out == 0);
			assert(strm.avail_in == 0);     /* all input will be used */

			/* done when last data in file processed */
		} while (flush != Z_FINISH);
		assert(ret == Z_STREAM_END);        /* stream will be complete */

		/* clean up and return */
		(void)deflateEnd(&strm);
		return Z_OK;
	}

	public:

		operator int() const {return m_nFile;}

		~CFile()
		{
			if (m_pFile)
				fclose(m_pFile);
			else if (m_nFile)
				close(m_nFile);
		}
		CFile(string strPath, bool bGzip, string strMode = "rb") :
			m_nFile(0), m_bGzip(bGzip)
		{
			FillContentType(strPath);

			Open(strPath, strMode);
		}

		void Open(string strPath, string strMode = "r+b")
		{
			size_t nPos = strPath.rfind('/');
			if (nPos < strPath.length()-1)
				m_strFileName = strPath.substr(nPos);


			m_nFile = 0;
			if ((!m_strContentType.length()) || (!m_bGzip))
			{
				DEBUG_LOG("open not gzipped file");
				m_pFile = fopen(strPath.c_str(), strMode.c_str());
			}
			else
			{
				string strGzipPath = "./gziped_files/"+html::utils::md5(m_strFileName);

				DEBUG_LOG("try mkdir");
				_mkdir("./gziped_files", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

				Copy(strPath, strGzipPath);

				m_pFile = fopen(strGzipPath.c_str(), strMode.c_str());

				m_strContentType += "\r\nContent-Encoding: gzip";
			}
			if (m_pFile)
				m_nFile = _fileno(m_pFile);
			if (m_nFile != -1)
				fstat(m_nFile, &stat_buf);
		}

		static void Copy(string strFrom, string strTo)
		{
			FILE *pFrom = fopen(strFrom.c_str(), "rb");
			if (!pFrom)
				return;
			int nFrom = _fileno(pFrom);
			if (!nFrom)
			{
				fclose(pFrom);
				return;
			}

			struct stat statFrom, statTo;
			fstat(nFrom, &statFrom);

			FILE *pTo = fopen(strTo.c_str(), "r+b");
			if (pTo)
			{
				int nTo = _fileno(pTo);
				if (nTo)
				{
					fstat(nTo, &statTo);
					if (statFrom.st_mtime == statTo.st_mtime)
					{
						fclose(pFrom);
						fclose(pTo);
						return;
					}
				}
				fclose(pTo);
			}

			pTo = fopen(strTo.c_str(), "w+b");

			if (!pTo)
			{
				fclose(pFrom);
				return;
			}

			//ProcessCopy(pFrom, statFrom.st_size, pTo);
			def(pFrom, pTo);
			fclose(pFrom);
			fclose(pTo);

			struct utimbuf times;
			times.actime = statFrom.st_atime;
			times.modtime = statFrom.st_mtime;
			utime(strTo.c_str(), &times);
		}

		const off_t GetFileSize() const
		{
			return stat_buf.st_size;
		}
		inline void FillContentType(string strPath)
		{
			string strEncodding = "charset=utf-8";
			if ((strPath.rfind(".htm") == strPath.length()-4) || (strPath.rfind(".html") == strPath.length()-5))
				m_strContentType = "text/html; " + strEncodding;
			else if (strPath.rfind(".txt") == strPath.length()-4)
				m_strContentType = "text/plain " + strEncodding;
			else if (strPath.rfind(".css") == strPath.length()-4)
				m_strContentType = "text/css; " + strEncodding;
			else if (strPath.rfind(".xml") == strPath.length()-4)
				m_strContentType = "text/xml; " + strEncodding;
			else if (strPath.rfind(".js") == strPath.length()-3)
				m_strContentType = "application/javascript; " + strEncodding;
			else
				m_strContentType = "application/octet-stream";
		}
		const string GetContentType() const
		{
			return m_strContentType;
		}
		const bool IsOpen () const {return ((m_pFile!=0) || (m_nFile!=0));}
		inline size_t ReadBuffer(void *pBuffer, size_t nCount, size_t startPosition)
		{
			if (!m_pFile)
			{
				d_printf("ERROR: file handle is NULL\n");
				return 0;
			}

			if (fseek(m_pFile, startPosition, SEEK_SET) != 0)
			{
				d_printf("ERROR: seek error\n");
				return 0;
			}

			size_t ret = fread(pBuffer, 1, nCount, m_pFile);
			if (ret == 0)
			{
				d_printf("fread return 0 (nCount=%i)\n", nCount);
			}
			return ret;
		}
	private:
		int m_nFile;
		struct stat stat_buf;
		string m_strContentType;
		FILE *m_pFile;
	};
	template <class startupType>
	class CInterface
	{
	public:
		startupType m_StartupInfo;

		CInterface(startupType info) :
			m_StartupInfo(info)
		{
		}

		fd_set m_readfds, m_writefds, m_exceptfds;

		vector<struct epoll_event> m_events;
	};
	class CSocket
	{
		SSL_CTX* m_pSSLContext;
		SSL* m_pSSL;

		explicit CSocket(const CSocket &socket) {int *p = new int;}

	public:
		bool ShutdownSocket(int nHow = SD_BOTH)
		{
			DEBUG_LOG("ShutdownSocket\n");
			if (m_hSocket == INVALID_SOCKET)
				return true;

			shutdown(m_hSocket, nHow);
			//m_bConnected = false;
			return true;
		}
		void Cleanup()
		{
			m_bAllRecieved = false;
			m_nAllRecieved = 0;
			m_nSendPosition = 0;
			m_readBuffer.clear();
			m_sendBuffer.clear();
		}
	private:
		bool CloseSocket()
		{
			DEBUG_LOG("CloseSocket m_hSocket=%i\n", GetHandle());
			m_bConnected = false;
			if (m_hSocket == INVALID_SOCKET)
				return true;

			ShutdownSocket();

			closesocket(m_hSocket);
			m_hSocket = INVALID_SOCKET;
			return true;
		}

		bool m_bSSL;

	public:
		const bool IsSSL() const {return m_bSSL;}

#ifndef _DEBUG
		CSocket(int nPort, bool bIsSSL) :
			m_hSocket(socket(PF_INET6, SOCK_STREAM, 0)), m_nPort(nPort), m_bSSL(bIsSSL), m_pSSL(NULL), m_pSSLContext(NULL)
		{
			if (m_hSocket == INVALID_SOCKET)
				return;

			if (m_bSSL)
				InitSSL();

			if (m_hSocket == INVALID_SOCKET)
				return;

			cout << "socket created\n";

			SET_NONBLOCK(m_hSocket);

			struct sockaddr_in6 local;
			memset(&local, 0, sizeof(local));

			local.sin6_family = AF_INET6;
			local.sin6_port = htons(nPort);
			local.sin6_addr = in6addr_any;

			const char on = 1;
			setsockopt(m_hSocket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
			setsockopt(m_hSocket, IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(on));
			if ( ::bind(m_hSocket, (struct sockaddr*)&local, sizeof(local)) == SOCKET_ERROR)
			{
				DEBUG_LOG("bind error = %i", errno);
				CloseSocket();
				return;
			}
			cout << "socket binded\n";

			//if (m_bSSL)
			//	return;
			if (listen(m_hSocket, SOMAXCONN) == SOCKET_ERROR)
			{
				DEBUG_LOG("listen error = %i", errno);
				CloseSocket();
				return;
			}
			cout << "listen - ok\n";
		}
#else
		CSocket(int nPort, bool bIsSSL) :
			m_hSocket(socket(PF_INET, SOCK_STREAM, 0)), m_nPort(nPort), m_bSSL(bIsSSL), m_pSSL(NULL), m_pSSLContext(NULL)
		{
			if (m_hSocket == INVALID_SOCKET)
				return;

			if (m_bSSL)
				InitSSL();

			if (m_hSocket == INVALID_SOCKET)
				return;

			cout << "socket created\n";

			SET_NONBLOCK(m_hSocket);

			struct sockaddr_in local;
			memset(&local, 0, sizeof(local));

			local.sin_family = AF_INET;
			local.sin_port = htons(nPort);
			local.sin_addr.s_addr = htons( INADDR_ANY );

			if ( ::bind(m_hSocket, (struct sockaddr*)&local, sizeof(local)) == SOCKET_ERROR)
			{
				DEBUG_LOG("bind error = %i", errno);
				CloseSocket();
				return;
			}
			cout << "socket binded\n";

			if (listen(m_hSocket, SOMAXCONN) == SOCKET_ERROR)
			{
				DEBUG_LOG("listen error = %i", errno);
				CloseSocket();
				return;
			}
			cout << "listen - ok\n";
		}
#endif

		void InitSSL()
		{
			auto *meth = SSLv23_server_method();

			m_pSSLContext = SSL_CTX_new (meth);
			if (SSL_CTX_use_certificate_chain_file(m_pSSLContext, CERTF) <= 0)
				ERR_print_errors_fp(stderr);

//			SSL_CTX_set_default_passwd_cb_userdata(m_pSSLContext, (void*)(sKeyPassWord.c_str()));

			if (SSL_CTX_use_PrivateKey_file(m_pSSLContext, KEYF, SSL_FILETYPE_PEM) <= 0)
				ERR_print_errors_fp(stderr);

			if (!SSL_CTX_check_private_key(m_pSSLContext))
			{
				DEBUG_LOG("error in check_private_key");
				CloseSocket();
				//cout << "error in check_private_key\n";
				//fprintf(stderr,"Private key does not match the certificate public key\n");
			}
		}

		CSocket(const SOCKET hAcceptedSocket, const struct sockaddr_in6 *pSockAddr, bool bIsSSL) :
			m_hSocket(hAcceptedSocket), m_bConnected(true), m_nPort(pSockAddr->sin6_port), m_addr(pSockAddr->sin6_addr), m_bSSL(bIsSSL),
			m_pSSL(NULL), m_pSSLContext(NULL)
		{
			if (m_bSSL)
				InitSSL();
		}
		~CSocket()
		{
			DEBUG_LOG("Destruct CSocket");
			ShutdownSocket();
			CloseSocket();
			
			if (m_pSSL)
				SSL_free (m_pSSL);
			if (m_pSSLContext)
				SSL_CTX_free (m_pSSLContext);
		}

		const int GetHandle() const {return m_hSocket;}


#ifdef _WIN32
	#if (_MSC_VER < 1700)
		static const char* inet_ntop(int af, const void* src, char* dst, int cnt)
		{
 
			SOCKADDR_IN6 srcaddr;
 
			memset(&srcaddr, 0, sizeof(struct sockaddr_in));
			memcpy(&(srcaddr.sin6_addr), src, sizeof(srcaddr.sin6_addr));
 
			srcaddr.sin6_family = af;
			if (WSAAddressToString((struct sockaddr*) &srcaddr, sizeof(SOCKADDR_IN6), 0, dst, (LPDWORD) &cnt) != 0) {
				DWORD rv = WSAGetLastError();
				printf("WSAAddressToString() : %d\n",rv);
				return NULL;
			}
			return dst;
		}			//InetNtop(AF_INET6, (void *)&m_addr, saddr, INET6_ADDRSTRLEN);
	#endif
#endif
		const string GetIP() const
		{
			char saddr[INET6_ADDRSTRLEN+10];

			inet_ntop(AF_INET6, (void *)&m_addr, saddr, INET6_ADDRSTRLEN);

			return saddr;
		}

		const SOCKET Accept(struct sockaddr_in6 *pServer) const
		{
			assert (m_hSocket != INVALID_SOCKET);

			socklen_t nLen = sizeof(*pServer);

			return accept(m_hSocket, (struct sockaddr*)pServer, &nLen);
		}

		const bool IsAccepted()
		{
			if ((!m_bSSL) && IsConnected())
				return true;
			
			if (!m_pSSLContext)
			{
				return false;
			}

			if (!m_pSSL)
			{
				m_pSSL = SSL_new (m_pSSLContext);
				
				if (!m_pSSL)
				{
					return false;
				}

				SSL_set_fd (m_pSSL, m_hSocket);
			}

			const int err = SSL_accept (m_pSSL); 
			if (err == 1)
				return true;

			const int nCode = SSL_get_error(m_pSSL, err);
			
			if ((nCode == SSL_ERROR_WANT_READ) || (nCode == SSL_ERROR_WANT_WRITE))
			{
				return false;
			}

			DEBUG_LOG("!IsAccepted");
			ShutdownSocket();
			CloseSocket();
			return false;
		}
		const bool GetSertificate()
		{
			if (!m_pSSLContext || !m_pSSL)
				return false;
			
			/* Get client's certificate (note: beware of dynamic allocation) - opt */

			X509* client_cert = SSL_get_peer_certificate (m_pSSL);
			if (client_cert != NULL) 
			{
				d_printf ("Client certificate:\n");
    
				char* str = X509_NAME_oneline (X509_get_subject_name (client_cert), 0, 0);
				if (!str)
					return false;

				d_printf ("\t subject: %s\n", str);
				OPENSSL_free (str);
    
				str = X509_NAME_oneline (X509_get_issuer_name  (client_cert), 0, 0);
				if (!str)
					return false;

				d_printf ("\t issuer: %s\n", str);
				OPENSSL_free (str);
    
				/* We could do all sorts of certificate verification stuff here before
					deallocating the certificate. */
    
				X509_free (client_cert);
			} 
			else
				d_printf ("Client does not have certificate.\n");

			return true;
		}
		inline bool InitRecv(int nMaxLen)//, string strEndString)
		{
			m_nAllRecieved = 0;
			m_bAllRecieved = false;
			m_nMaxLength = nMaxLen;
			m_readBuffer.clear();

			bool bRet = false;
			if (m_bSSL)
				bRet = GetSertificate();
			else
				bRet = ReadSocket();

			return bRet;
		}
		bool ReadSSL()
		{
			if (!m_pSSLContext || !m_pSSL)
				return false;

			static char szReadingBuffer[100000];
			
			const int err = SSL_read (m_pSSL, szReadingBuffer, 100000);
			if (err > 0)
			{
				m_readBuffer.resize(m_nAllRecieved+err);
				memcpy(&m_readBuffer[m_nAllRecieved], szReadingBuffer, err);
				m_nAllRecieved += err;

				m_bAllRecieved = true;
				return true;
			}

			const int nCode = SSL_get_error(m_pSSL, err);
			
			if (nCode == SSL_ERROR_NONE)
			{
				m_bAllRecieved = true;
				return true;
			}

			if ((nCode != SSL_ERROR_WANT_READ) && (nCode != SSL_ERROR_WANT_WRITE))
				return false;
			
			return true;
		}

		inline bool ReadSocket()
		{
			if ((m_hSocket == INVALID_SOCKET) || (!m_bConnected))
			{
				d_printf("error: invalid socket\n");
				return false;
			}

			if (m_bSSL)
				return ReadSSL();

			static char szReadingBuffer[100000];
			//m_readBuffer.clear();
//			while(true)
//			{
				int nBuffLen = 100000;

				//if (m_nAllRecieved+nBuffLen > m_nMaxLength)
				//	nBuffLen = m_nMaxLength - m_nAllRecieved;

				memset(szReadingBuffer, 0, nBuffLen+10);

				d_printf("try recv len=%i\n", nBuffLen);
				errno = 0;
				int n = recv(m_hSocket, szReadingBuffer, nBuffLen, 0);
				d_printf("recv returned n=%i\n", n);

#ifdef _DEBUG
				DEBUG_LOG("recv returned n=%i\n", n);
//				if (n < 100000)
//					DEBUG_LO
#endif

				int nError = WSAGetLastError();

		#ifndef _WIN32
				if (nError == 115)
					nError = WSAEWOULDBLOCK;
		#endif
				if ((nError != WSAEWOULDBLOCK) && (nError != S_OK))
				{
					//ShutdownSocket();
					//CloseSocket();
					d_printf("error: recv n=%i, errno=%i\n", n, nError);

					return false;
				}
				if (n == SOCKET_ERROR)
					return true;

				if (n == 0)
				{
					DEBUG_LOG("recv return 0 errno=%i", nError);
					m_bConnected = false;
					m_bAllRecieved = true;

					if (m_nAllRecieved == 0)
					{
						d_printf("error: recv return 0 errno=%i\n", nError);
						return false;
					}

					/*if (m_nAllRecieved != 0)
					{
						m_bAllRecieved = true;
						m_bConnected = false;
					}*/

					return true;
				}

				m_readBuffer.resize(m_nAllRecieved+n);
				memcpy(&m_readBuffer[m_nAllRecieved], szReadingBuffer, n);
				m_nAllRecieved += n;

				if (m_nAllRecieved >= m_nMaxLength)
				{
					m_bAllRecieved = true;
					return true;
				}

				if (nError == S_OK)
				{
					m_bAllRecieved = true;
					return true;
				}
//			}
			return true;
		}
		inline bool IsAllReaded(string strEnd, unsigned long long llLength)
		{
			if (m_nAllRecieved >= llLength)
				return true;

			string strContent = GetReadedContent();
			if ((strEnd.length()) && (strContent.rfind(strEnd) == strContent.npos))
				m_bAllRecieved = false;

			return m_bAllRecieved;
		}
		void GetReadedContent(vector<BYTE> *pBuffer) const
		{
			(*pBuffer) = m_readBuffer;
		}
		const string GetReadedContent(int nStartPos = 0) const
		{
			string strContent;
			if ((!m_nAllRecieved) || (nStartPos >= m_nAllRecieved))
				return strContent;

			if (!m_readBuffer[nStartPos])
				return "";

			vector<BYTE> ret = m_readBuffer;
			ret.push_back(0);

			return (const char*)&(ret[nStartPos]);
		}

		inline void InitSend()
		{
			m_nSendPosition = 0;
			m_sendBuffer.clear();
		}

		const ACTION SendVector(vector<BYTE> *pVector)
		{
			if (!pVector)
				return A_ERROR;

			if (!pVector->size())
				return A_ALL_SENDED;

			if (!SendBuffer((const char *)&((*pVector)[0]), pVector->size()))
				return A_ERROR;

			return A_SENDING;
		}

		inline ACTION SendFile(const shared_ptr<CFile> pFile, size_t *pnFilePos)
		{
#ifdef _SEND_FILE
			if (!m_bSSL)
			{
				ssize_t sended = sendfile(m_hSocket, (int)(*pFile), (off_t *)pnFilePos, 1000000);
				if (sended == -1)
				{
					d2_printf("sendfile error = %i\n", errno);
					//exit(0);
					return A_ERROR;
				}

				d_printf("filepos = %i, size = %i\n", *pnFilePos, pFile->GetFileSize());
				if (pFile->GetFileSize() != *pnFilePos)
					return A_SENDING;
				return A_ALL_SENDED;
			}
#endif

			static BYTE buffer[100005];

			size_t stReaded = pFile->ReadBuffer(buffer, 10000, *pnFilePos);
			*pnFilePos += stReaded;

			if (stReaded == 0)
				return A_ALL_SENDED;

			if (!SendBuffer((const char*)buffer, stReaded))
				return A_ERROR;

			return A_SENDING;
		}

		inline bool SendBuffer(const char *pszBuffer, int nBuffLen)
		{
			/*string strTemp;
			strTemp.assign(pszBuffer, nBuffLen);
			DEBUG_LOG("%s", strTemp.c_str());*/
			d_printf("SendBuffer\n");
			if (m_hSocket == INVALID_SOCKET)
			{
				d2_printf("error: SendBuffer: (m_hSocket == INVALID_SOCKET)\n");
				return false;
			}

			m_sendBuffer.resize(nBuffLen);
			memcpy(&m_sendBuffer[0], pszBuffer, nBuffLen);
#ifdef _DEBUG
			//d_printf("buffer=%s\n", pszBuffer);
#endif

			bool bRet = ContinueSend();
			if (!bRet)
			{
				d2_printf("error: SendBuffer: (!ContinueSend())\n");
				DEBUG_LOG("error: SendBuffer: (!ContinueSend())");
			}

			return bRet;
		}
		const bool SendSSL()
		{
			if (!m_pSSLContext || !m_pSSL)
				return false;

			int err = SSL_write (m_pSSL, (const char*)&m_sendBuffer[m_nSendPosition], m_sendBuffer.size()-m_nSendPosition);
			if (err > 0)
			{
				m_nSendPosition += err;

				//Åñëè óäàëîñü ïîñëàòü âñå äàííûå, òî ïåðåõîäèì ê ñëåäóþùåìó ñîñòîÿíèþ
				if ((size_t)err == m_sendBuffer.size())
					return true;

				//Åñëè îòîñëàëè íå âñå äàííûå, òî îñòàâèì â áóôåðå òîëüêî òî, ÷òî åùå íå ïîñëàíî
				vector<unsigned char> vTemp(m_sendBuffer.size()-err);
				memcpy(&vTemp[0], &m_sendBuffer[err], m_sendBuffer.size()-err);
				m_sendBuffer = move(vTemp);

				return true;
			}
	 
			const int nCode = SSL_get_error(m_pSSL, err);
			if ((nCode != SSL_ERROR_WANT_READ) && (nCode != SSL_ERROR_WANT_WRITE))
				return false;

			return true;
		}
		inline bool ContinueSend()
		{
			if (IsAllSended())
				return true;

			if (m_bSSL)
				return SendSSL();

			errno = 0;
			int n = send(m_hSocket, (const char*)&m_sendBuffer[m_nSendPosition], m_sendBuffer.size()-m_nSendPosition, 0);
			//int n = send(m_hSocket, (const char*)&m_sendBuffer[m_nSendPosition], min(1, m_sendBuffer.size()-m_nSendPosition), 0);

#ifdef _DEBUG

			if (n > 0)
			{
				string strTemp;
				strTemp.assign((const char*)&m_sendBuffer[m_nSendPosition], n);
				string strTemp0 = "\r\n=====================================================\r\n"+strTemp+"\r\n=====================================================\r\n";
				DEBUG_LOG("%s", strTemp0.c_str());
			}
			else
			{
				DEBUG_LOG("n = SOCKET_ERROR");
			}
#endif

			int nErr = WSAGetLastError();
			if (n != SOCKET_ERROR)
				m_nSendPosition += n;
			else if (nErr != WSAEWOULDBLOCK)
			{
				DEBUG_LOG("error: ContinueSend: errno=%i\n", errno);
				return false;
			}

			return true;
		}
		inline int GetSendPosition() {return m_nSendPosition;}
		inline bool IsAllSended()
		{
			return (m_nSendPosition >= m_sendBuffer.size());
		}

		inline bool IsConnected() {return m_bConnected;}

		inline operator SOCKET() {return m_hSocket;}
	private:
		SOCKET m_hSocket;
		bool m_bConnected;
		int m_nPort;
		struct in6_addr m_addr;

		int m_nAllRecieved;
		bool m_bAllRecieved;
		//string m_strEndString;
		int m_nMaxLength;

		vector<BYTE> m_readBuffer;
		vector<BYTE> m_sendBuffer;

		int m_nSendPosition;
	};

	//template <class startupType>
	class CHttpHeader
	{
		vector<BYTE> m_vCurrentBody;
		unsigned long long m_llBodyLength, m_llContentLength;
		map<string, string> m_mapNameToValue;
		vector<string> m_vQueryes;
		string m_strURI, m_strQuery, m_strBoundary;
		static size_t ParseLine(int nPos, string &strLines, string &strFirst, string &strSecond)
		{
			size_t nRet = strLines.length();
			string substr = strLines.substr(nPos);
			if (substr.length() < 4)
				return nRet;

			int nEnd = substr.find("\r\n");
			if ((nEnd == substr.npos) || (nEnd == 0))
				return nRet;

			nRet = nPos + nEnd+2;

			string strLine = substr.substr(0, nEnd);

			int nSpace = strLine.find(" ");
			if (nPos > 0)
				nSpace = strLine.find(": ");

			if (nSpace != strLine.npos)
				strFirst = strLine.substr(0, nSpace);
			else
				nSpace = -1;

			string substr2 = strLine.substr(nSpace+1);
			if (nPos != 0)
			{
				strSecond = substr2;
				return nRet;
			}
			nSpace = substr2.find(" ");
			if (nSpace == substr2.npos)
				strSecond = substr2;
			else
				strSecond = substr2.substr(0, nSpace);

			return nRet;
		}
	public:
		CHttpHeader()
		{
			Clear();
			m_mapNameToValue["Method"] = "";
		}
		inline void Clear()
		{
			m_vCurrentBody.clear();
			m_mapNameToValue.clear();
			m_vQueryes.clear();
			m_strURI = m_strQuery = m_strBoundary = "";
			m_llBodyLength = m_llContentLength = 0;
		}
		const map<string, string> &GetHeadersMap() const {return m_mapNameToValue;}
		inline unsigned long long GetBodyLength() {return m_llBodyLength;}
		inline string GetURI() const {return m_strURI;}
		inline string GetMethod() {return m_mapNameToValue["Method"];}
		inline string GetValue(string strKey) const
		{
			string strRet;
			if (m_mapNameToValue.find(strKey) != m_mapNameToValue.end())
				strRet = m_mapNameToValue.at(strKey);
			return strRet;
		}
		inline unsigned long long GetContentLength()
		{
			if (m_llContentLength)
				return m_llContentLength;

			string strLen = GetValue("Content-Length");
			if (!strLen.length())
				return 0;

			std::stringstream ss(strLen);
			unsigned long long ret;
			ss >> ret;

			m_llContentLength = ret;
			return ret;
		}
		inline string GetQuery() {return m_strQuery;}
		inline vector<string> GetQueryes() {return m_vQueryes;}
		inline string operator[] (string strName) {return m_mapNameToValue[strName];}
		static string GetKey(string strKeyAndVal)
		{
			string strRet;
			int nPos = strKeyAndVal.find("=");
			if ((nPos != strKeyAndVal.npos) && (nPos > 0))
				strRet = strKeyAndVal.substr(0, nPos);
            return strRet;
		}
		static string GetVal(string strKeyAndVal)
		{
			string strRet;
			int nPos = strKeyAndVal.find("=");
			if ((nPos != strKeyAndVal.npos) && (nPos+1 != strKeyAndVal.npos))
				strRet = strKeyAndVal.substr(nPos+1);
            return strRet;
		}
		map<string, string> ParseQueries() const
		{
			map<string, string> ret;
			for (size_t n=0; n<m_vQueryes.size(); n++)
			{
				ret[GetKey(m_vQueryes[n])] = GetVal(m_vQueryes[n]);
			}
			return ret;
		}
		inline vector<BYTE> GetCurrentBody(bool bClean = true)
		{
			vector<BYTE> ret = m_vCurrentBody;
			m_vCurrentBody.clear();
			return ret;
		}
		const string GetBoundary() const {return m_strBoundary;}
		const string GetContentType() const
		{
			if (m_mapNameToValue.find("Content-Type") == m_mapNameToValue.end())
				return "";
			return m_mapNameToValue.find("Content-Type")->second;
		}
		inline void SetCurrentBody(BYTE *pBuffer, int nLength)
		{
			if (nLength <= 0)
				return;

			m_llBodyLength += nLength;

			m_vCurrentBody.resize(nLength);
			memcpy(&m_vCurrentBody[0], pBuffer, nLength);
		}
		inline string SplitHeaderAndBody(vector<BYTE> *pBuffer)
		{
			string strHeader = (const char*)&((*pBuffer)[0]);
			int nPos = strHeader.find("\r\n\r\n");
			if ((nPos == strHeader.length()-4) ||
				(nPos == strHeader.npos))
				return strHeader;

			string strRet = strHeader.substr(0, nPos+4);

			if (pBuffer->size()-nPos-4)
				SetCurrentBody(&((*pBuffer)[nPos+4]), pBuffer->size()-nPos-4);

			return strRet;
		}
		string m_strHeader;
		void Parse(const shared_ptr<CSocket> pSocket, const string &strIndex)
		{
			Clear();

			vector<BYTE> vReadedBuffer;
			pSocket->GetReadedContent(&vReadedBuffer);

			m_strHeader = SplitHeaderAndBody(&vReadedBuffer);

			size_t nPos = 0;
			while (1)
			{
				string strFirst, strSecond;
				nPos = ParseLine(nPos, m_strHeader, strFirst, strSecond);

				if (nPos == m_strHeader.length())
					break;

				if ((strFirst == "GET") || (strFirst == "POST") || (strFirst == "HEAD"))
				{
					m_mapNameToValue["Method"] = strFirst;
					int nPos = strSecond.find("://");
					if (nPos == strSecond.npos)
						utils::Replace<string>(strSecond, 0, "//", "/");
					else
						utils::Replace<string>(strSecond, nPos+3, "//", "/");
				}
				if (strFirst == "Content-Type" || strFirst == "content-type")
				{
					if (strSecond.find("multipart/form-data") != strSecond.npos)
					{
						size_t nPos1 = strSecond.find("boundary=");
						if ((nPos1 != strSecond.npos) && (nPos1+9 < strSecond.length()))
							m_strBoundary = strSecond.substr(nPos1+9);
					}
					strFirst = "Content-Type";
				}

				m_mapNameToValue[strFirst] = strSecond;
			}

			string strFullAddress = m_mapNameToValue[m_mapNameToValue["Method"]];

			nPos = strFullAddress.find("?");
			if (nPos == strFullAddress.npos)
			{
				m_strURI = strFullAddress;
			}
			else
			{
				m_strURI = strFullAddress.substr(0, nPos);
				m_strQuery = strFullAddress.substr(nPos+1);
			}

			if (m_strURI.find("..") != m_strURI.npos)
				m_strURI = "/";

			if (m_strURI.rfind('/') == m_strURI.length()-1 && m_strURI.find("http://") == -1)
			{
				if (m_strHeader.find("." DNS_NAME) == -1 || m_strHeader.find(": www." DNS_NAME) != -1)
					m_strURI += strIndex;//pServer->m_StartupInfo.GetIndex();
			}

			m_vQueryes = ParseQuery(m_strQuery);
		}

		static vector<string> ParseQuery(const string &strQuery)
		{
			vector<string> ret;
			if (!strQuery.length())
				return ret;

			string strLast = strQuery;
			while(1)
			{
				string strTemp = strLast;
				int nPos = strTemp.find("&");
				if (nPos == -1)
				{
					ret.push_back(strTemp);
					break;
				}

				ret.push_back(strTemp.substr(0, nPos));

				strLast = strTemp.substr(nPos+1);
			}
			return ret;
		}
	};
	class CProxyInfo
	{
		//time_t m_tmLastAccess;
	public:
		enum STATE {STATE_NULL, STATE_TRUE, STATE_FALSE };
		STATE m_stCurrent;
		//string m_strCurrentPage;
		//bool m_bCapcha;
		CProxyInfo() : m_stCurrent(STATE_NULL) {}

		//int m_nAccessCounter;
		//time_t GetLastAccessTime() const {return m_tmLastAccess;}
	};
	extern unordered_map<string, shared_ptr<CProxyInfo> > g_mapProxyInfo;

	template <class startupType>
	class CClient
	{
		uint64_t m_uiAllSended;
		class CAction
		{
			ACTION m_nAction;
			int m_nInstance;
			time_t m_tmLastTime;
		public:
			CAction() : m_nAction(A_NULL)
			{
				static int instance = 0;
				m_nInstance = instance++;
				m_tmLastTime = time(0);
			}
			void Set(ACTION n) 
			{
				if (n == A_ERROR)
				{
					DEBUG_LOG("Set action to A_ERROR");
				}
			//	DEBUG_LOG("SetAction for id=%i", m_nInstance);
				m_nAction = n;
			}
			void ResetTimeout()
			{
				m_tmLastTime = time(0);
			}
			const ACTION Get() const {return m_nAction;}
			const int id() const {return m_nInstance;}
			const bool IsTimeout() const
			{
#ifdef _DEBUG
				return false;
#endif
				return (time(0)-m_tmLastTime > 30);
			}
		};

		CHttpHeader m_httpHeader;
		size_t m_nBytesReaded;

		shared_ptr<CFile> m_pFile;

		explicit CClient(const CClient &client) {}
		void Cleanup()
		{
			m_bGzip = m_bHeaderReaded = m_bHeaderSended = m_bKeepAlive = m_bIsMethodPost = false;
			m_tmLastCGIDataTime = 0;

			m_nFilePos = 0;
			m_nResponceHeaderLength = 0;
//			m_pCGIPipe = NULL;
			m_nBytesReaded = 0;

			m_httpHeader.Clear();

			m_llResponceContentLength = m_llCurrentSendLength = 0;

			m_pSocket->Cleanup();
			m_pFile.reset();
		}

		void RedirectToCapcha()
		{
			string strCapcha = 
					"<html>"
						"<head><script src='https://www.google.com/recaptcha/api.js'></script></head>"
						"<body>"
							"<form method=get>"
								"<input type=hidden name='url' value='"+m_httpHeader.GetURI()+"'>"
								"<div class='g-recaptcha' data-sitekey='6Lc8rQYTAAAAAGZjf1H8giiVEIqqDTFlZ2ZddSsm'></div>"
								"<input type=submit value='OK'>"
							"</form>"
						"</body>"
					"</html>";

			ostringstream str;
			str << "HTTP/1.1 200 OK\r\n"
					<< "Content-Type: text/html\r\n"
					<< "Content-Length: " << strCapcha.length() << "\r\n"
					<< "Connection: close\r\n" 
					<< "\r\n" << 
					strCapcha.c_str();

			m_strResponceHeader = str.str();
		}

		void RedirectToURL(const string strURL)
		{
				ostringstream str;
				str << "HTTP/1.1 302 Moved Temporarily\r\n"
					<< "Location: " << strURL.c_str() << "\r\n"
					<< "Content-Length: " << 0 << "\r\n"
					<< "Connection: close\r\n" 
					<< "\r\n";
				m_strResponceHeader = str.str();
		}

		bool IsCapchaOK(string strCapcha) const
		{
			string strCommand = 
#ifdef _DEBUG
				"C:\\Users\\Kostja\\YandexDisk\\english_simple_server\\curl\\release\\curl.exe --insecure "
#else
				"curl --insecure " 
#endif
				"--data \"secret=6Lc8rQYTAAAAAOA-tOnIHwzUSh7mRpoldtpWYnnl&response="+strCapcha+"\" " +
				"https://www.google.com/recaptcha/api/siteverify";

#ifdef _DEBUG
			FILE *lsofFile_p = popen(strCommand.c_str(), "rb");
#else
			FILE *lsofFile_p = popen(strCommand.c_str(), "r");
#endif

			if (!lsofFile_p)
				 return false;

			string strRet;

			char buffer[10];
			memset(buffer, 0, 10);
			while (fgets(buffer, sizeof(buffer)-1, lsofFile_p))
			{
				strRet += buffer;
				memset(buffer, 0, 10);
			}
			pclose(lsofFile_p);

			if (strRet.find("\"success\": true") != -1)
				return true;

			return false;
		}

		bool ProxyRedirect()
		{
			auto it = g_mapProxyInfo.find(GetIP());
			if (it == g_mapProxyInfo.end())
				g_mapProxyInfo[GetIP()] = shared_ptr<CProxyInfo>(new CProxyInfo);

			it = g_mapProxyInfo.find(GetIP());
			if (it->second->m_stCurrent == CProxyInfo::STATE_NULL)
			{
				RedirectToCapcha();

				it->second->m_stCurrent = CProxyInfo::STATE_FALSE;
				return true;
			}

			//string strRef
			if (it->second->m_stCurrent == CProxyInfo::STATE_FALSE)
			{
				map<string, string> mapQueries = m_httpHeader.ParseQueries();

				auto responce = mapQueries.find("g-recaptcha-response");
				auto url = mapQueries.find("url");

				if (url != mapQueries.end())
				{
					if ((responce == mapQueries.end()) || !IsCapchaOK(responce->second))
					{
						RedirectToCapcha();
						it->second->m_stCurrent = CProxyInfo::STATE_NULL;
					}
					else
					{
						RedirectToURL(url->second);
						it->second->m_stCurrent = CProxyInfo::STATE_TRUE;
					}

					return true;
				}
			}
			//if (it->second->m_stCurrent = STATE_TRUE)
			
			return false;
		}

		static bool IsBlacklistedRequest(const CHttpHeader &header)
		{
			string strUserAgent = header.GetValue("User-Agent");
			if ((strUserAgent.find("python-requests") != strUserAgent.npos) && (strUserAgent.find("CPython") != strUserAgent.npos))
			{
				d2_printf("error: Python bot disable\n");
				return true;
			}
			if (strUserAgent.find("www.sentibot.eu") != strUserAgent.npos)
			{
				d2_printf("error: www.sentibot.eu bot disable\n");
				return true;
			}

			const string strReferer = header.GetValue("Referer");
			if (strReferer.find("www.wspack.kr") != strReferer.npos)
			{
				d2_printf("error: www.wspack.kr bot disable\n");
				return true;
			}

			vector<string> vBadURI;
			vBadURI.push_back("wp-login.php");

			const string strURI = header.GetURI();

			for (size_t n=0; n<vBadURI.size(); n++)
			{
				if (strURI.find(vBadURI[n]) != -1)
				{
					DEBUG_LOG("found blacklisted URI string: %s", vBadURI[n].c_str());
					return true;
				}
			}
			return false;
		}
	public:

		const int id() const {return m_CurrentAction.id();}

		CClient(const SOCKET hAcceptedSocket, const struct sockaddr_in6 *pSockAddr, const bool bIsSSL) : m_bKeepAlive(false), m_uiAllSended(0)
		{
			
			SET_NONBLOCK(hAcceptedSocket);
			m_Event.data.fd = hAcceptedSocket;

			m_pSocket = shared_ptr<CSocket>(new CSocket(hAcceptedSocket, pSockAddr, bIsSSL));

			DEBUG_LOG("Accepted New Client id=%i; hSocket=%i", m_CurrentAction.id(), m_pSocket->GetHandle());
			Cleanup();
			m_CurrentAction.Set(A_ACCEPTED);
		}
		~CClient()
		{
			DEBUG_LOG("Close client id=%i", m_CurrentAction.id());
			Cleanup();
		}

		inline const bool IsTimeout() const { return m_CurrentAction.IsTimeout(); }
		inline bool Callback(CInterface<startupType> *pServer, const int nEvent = -1)
		{
			if ((SOCKET)*m_pSocket == INVALID_SOCKET)
			{
				DEBUG_LOG("error: (SOCKET)m_Socket == INVALID_SOCKET) id=%i", id());
				return false;
			}

			if (m_CurrentAction.Get() == A_ERROR)
			{
				DEBUG_LOG("error: (m_nCurrentAction == A_ERROR) id=%i", id());
				return false;
			}
			if (m_CurrentAction.IsTimeout())
			{
				DEBUG_LOG("Close Client (TIMEOUT) id=%i, uri=%s", id(), m_strURI.c_str());
				return false;
			}

			if (nEvent != -1)
			{
				if ((pServer->m_events[nEvent].events & EPOLLERR) ||
					(pServer->m_events[nEvent].events & EPOLLHUP))
				{
					if((!(pServer->m_events[nEvent].events & EPOLLIN)) &&
						(!(pServer->m_events[nEvent].events & EPOLLOUT)))
					{
						DEBUG_LOG("error: socket exception at epoll! id=%i", id());
						m_CurrentAction.Set(A_ERROR);
						return false;
					}
				}
			}
			else if (FD_ISSET(*m_pSocket, &pServer->m_exceptfds))
			{
				DEBUG_LOG("error: socket exception! id=%i", id());
				m_CurrentAction.Set(A_ERROR);
				return false;
			}

			switch(m_CurrentAction.Get())
			{
			case A_ACCEPTED:
				OnAccepted(pServer, nEvent);
				break;
			case A_READING:
				OnReading(pServer, nEvent, A_HEADER_READED);
				break;
			case A_HEADER_READED:
				OnHeaderReaded(pServer);
				break;
			case A_SENDING:
				OnSending(pServer, nEvent);
				break;
			case A_ALL_SENDED:
				return OnAllSended(pServer);
			case A_CGI_START:
				OnStartCGI(pServer);
				break;
			case A_CGI_CONTINUE:
				OnCGIContinue(pServer);
				break;
			case A_CGI_END:
				if (!m_bKeepAlive)
					m_pSocket->ShutdownSocket(SD_SEND);
				
				pServer->m_StartupInfo.EraseClient(id());
				
				if (m_bKeepAlive)
				{
					Cleanup();
					DEBUG_LOG("Keep-Alive set A_ACCEPTED id=%i", id());
					m_CurrentAction.Set(A_ACCEPTED);
					//StartReading();
				}
				else
				{
					DEBUG_LOG("NOT Keep-Alive return false id=%i", id());
					return false;
				}
					
				return true;
			}

			bool bRet = m_pSocket->IsConnected();
			if (!bRet)
			{
				DEBUG_LOG("error: !m_Socket.IsConnected() id=%i", id());
			}

			if (m_CurrentAction.Get() == A_ERROR)
			{
				DEBUG_LOG("Callback return with action A_ERROR id=%i", id());
				return false;
			}
			return bRet;
		}

		bool m_bGzip, m_bHeaderReaded, m_bKeepAlive, m_bIsMethodPost;
		struct epoll_event m_Event;

	private:
		shared_ptr<CSocket> m_pSocket;
		//FILE *m_pCGIPipe;
        CAction m_CurrentAction;

		size_t m_nFilePos;

		vector<BYTE> m_ResponceBody;
		unsigned long long m_llResponceContentLength, m_llCurrentSendLength;
		string m_strURI;

		string m_strResponceHeader;
		int m_nResponceHeaderLength;

		inline void StartReading()
		{
			m_CurrentAction.Set(A_READING);
			DEBUG_LOG("start reading id=%i", m_CurrentAction.id());

			if (!m_pSocket->InitRecv(1000000))
			{
				d2_printf("error: InitRecv failed\n");
				m_CurrentAction.Set(A_ERROR);
				return;
			}
			unsigned long long llMaxLen = 10000;
			string strEnd = "\r\n\r\n";
			if (m_bHeaderReaded)
			{
				strEnd = "";
				llMaxLen = m_httpHeader.GetContentLength() - m_httpHeader.GetBodyLength();
			}

			if (m_pSocket->IsAllReaded(strEnd, llMaxLen))
			{
				m_CurrentAction.Set(A_HEADER_READED);
				m_CurrentAction.ResetTimeout();
			}

			d_printf("recv init - ok; action=%i\n", A_HEADER_READED);
		}

		inline void OnAccepted(CInterface<startupType> *pServer, int nEvent)
		{
			if ((nEvent != -1) && (!(pServer->m_events[nEvent].events & EPOLLIN)))
			{
				//d_printf("OnAccepted nEvent=%i, events=%i\n", nEvent, pServer->m_events[nEvent].events);
				return;
			}
			else if ((nEvent == -1) && (!FD_ISSET(*m_pSocket, &pServer->m_readfds)))
				return;

			if (m_pSocket->IsAccepted())
				StartReading();
			//else
			//	m_CurrentAction.Set(A_ERROR);
		}
		inline void OnReading(CInterface<startupType> *pServer, int nEvent, ACTION next)
		{
			//DEBUG_LOG("OnReading id=%i", m_CurrentAction.id());
			if ((nEvent != -1) && (!(pServer->m_events[nEvent].events & EPOLLIN)))
				return;
			else if ((nEvent == -1) && (!FD_ISSET(*m_pSocket, &pServer->m_readfds)))
			{
				//if (m_bHeaderReaded)

				return;
			}

			if (!m_pSocket->ReadSocket())
			{
				d2_printf("error: ReadSocket failed\n");
				m_CurrentAction.Set(A_ERROR);
				return;
			}
			
			unsigned long long llMaxLen = 10000;
			string strEnd = "\r\n\r\n";
			if (m_bHeaderReaded)
			{
				strEnd = "";
				llMaxLen = m_httpHeader.m_strHeader.length() + m_httpHeader.GetContentLength();
			}

			if (m_pSocket->IsAllReaded(strEnd, llMaxLen))
				m_CurrentAction.Set(next);//A_HEADER_READED;
		}

		string m_strLastURI;
		inline void OnHeaderReaded(CInterface<startupType> *pServer)
		{
			d_printf("Header readed\n");
			DEBUG_LOG("******************************************************");
			DEBUG_LOG("New content readed id=%i", m_CurrentAction.id());//, m_pSocket->GetReadedContent().c_str());

			if (m_bHeaderReaded)
			{
				vector<BYTE> vBuffer;
				m_pSocket->GetReadedContent(&vBuffer);

				m_httpHeader.SetCurrentBody(&(vBuffer[0]), vBuffer.size());
				m_CurrentAction.Set(A_CGI_CONTINUE);

				m_bIsMethodPost = (m_httpHeader.GetMethod() == "POST");
				return;
			}

			DEBUG_LOG("New header readed id=%i", m_CurrentAction.id());//, m_pSocket->GetReadedContent().c_str());

			m_pSocket->InitSend();

			m_CurrentAction.Set(A_SENDING);

			m_bHeaderReaded = true;

			m_httpHeader.Clear();
			m_httpHeader.Parse(m_pSocket, pServer->m_StartupInfo.GetIndex());

			if ((IsBlacklistedRequest(m_httpHeader)) && (g_vWhiteListedIP.find(GetIP()) == g_vWhiteListedIP.end()))
			{
				DEBUG_LOG("client got blacklisted header and ip=%s not in whiltelist so client rejected", GetIP().c_str());
				m_CurrentAction.Set(A_ERROR);
				return;
			}

#if 0
			string strUserAgent = m_httpHeader.GetValue("User-Agent");
			if ((strUserAgent.find("python-requests") != strUserAgent.npos) && (strUserAgent.find("CPython") != strUserAgent.npos))
			{
				d2_printf("error: Python bot disable\n");
				m_CurrentAction.Set(A_ERROR);
				return;
			}
			if (strUserAgent.find("www.sentibot.eu") != strUserAgent.npos)
			{
				d2_printf("error: www.sentibot.eu bot disable\n");
				m_CurrentAction.Set(A_ERROR);
				return;
			}

			const string strReferer = m_httpHeader.GetValue("Referer");
			if (strReferer.find("www.wspack.kr") != strReferer.npos)
			{
				d2_printf("error: www.wspack.kr bot disable\n");
				m_CurrentAction.Set(A_ERROR);
				return;
			}
			/*if ((strUserAgent.find("Trident/") != strUserAgent.npos))
			{
				d2_printf("error: Trident bot disable\n");
				m_CurrentAction.Set(A_ERROR);
				return;
			}*/
#endif


			string strEncode = m_httpHeader.GetValue("Accept-Encoding");
			if (strEncode.find("gzip") != strEncode.npos)
				m_bGzip = true;
			
			const string strConnection = m_httpHeader.GetValue("Connection");
			m_bKeepAlive = //false;
				(strConnection.find("Keep-Alive") != strConnection.npos || 
				strConnection.find("keep-alive") != strConnection.npos) ? true : false;
			if (m_pSocket->IsSSL()) m_bKeepAlive = false;

			//string strConnection = m_httpHeader.GetValue("Connection");
			//if (strConnection.find(""))

			string strURI = pServer->m_StartupInfo.GetRoot().c_str();
			strURI += m_httpHeader.GetURI();//GetURIFromHeader(&pServer->m_strDefault);

			d_printf("Header readed URI=%s\n", strURI.c_str());

			if (!strURI.length())
			{
				d2_printf("error: (!strURI.length())\n");
				m_CurrentAction.Set(A_ERROR);
				return;
			}

			m_strLastURI = m_strURI = strURI;

			map<string, string> headers = m_httpHeader.GetHeadersMap();
			if (headers.find("Host") == headers.end() && headers.find("host") != headers.end())
				headers["Host"] = headers["host"];

			const string strPath = [](const string strURI) -> string
				{
					if (strURI.find("http://") != -1)
					{
						string strTmp = strURI.substr(strURI.find("http://")+7);
						if (strTmp.find("/") == -1)
							return "/";
						else
							return strTmp.substr(strTmp.find("/"));
					}
					return strURI;
				}(m_httpHeader.GetURI());

			//string strWWW
			if (headers["Host"].find("www." DNS_NAME) != -1)
			{   //redirect www.3s3s.org -> 3s3s.org
				ostringstream str;
				str << "HTTP/1.1 301 Moved Permanently\r\n"
					<< "Location: " << "http://" << DNS_NAME << strPath << "\r\n"
					<< "Content-Length: " << 0 << "\r\n"
					<< "Connection: close\r\n" 
					<< "\r\n";
				m_strResponceHeader = str.str();
				return;
			}
			if (m_pSocket->IsSSL() && (headers["Host"].find(DNS_NAME) != -1) && (headers["Host"].find(" " DNS_NAME) == -1))
			{
				string strHost = headers["Host"];
				while(strHost.find(" ") != -1)
				{
					strHost.replace(strHost.find(" "), 1, "");
				}
				
				ostringstream str;
				str << "HTTP/1.1 301 Moved Permanently\r\n"
						<< "Location: " << "http://h_t_t_p_s." << strHost << strPath << "\r\n"
						<< "Content-Length: " << 0 << "\r\n"
						<< "Connection: close\r\n" 
						<< "\r\n";
				m_strResponceHeader = str.str();
				return;
			}

			if (m_httpHeader.GetURI().find("http://") == 0)
			{ 
				if (ProxyRedirect())
					return;
			}

			if (pServer->m_StartupInfo.NeadCGI(m_strURI, m_httpHeader.GetQuery(), headers))
			{
				DEBUG_LOG("Nead CGI for url=%s", m_strURI.c_str());
				m_CurrentAction.Set(A_CGI_START);
				return;
			}
			DEBUG_LOG("NO PROXY for url=%s", m_strURI.c_str());

			m_pFile = shared_ptr<CFile>( new CFile(m_strURI, m_bGzip));
			//d_printf("file refs = %i\n", pServer->m_Files[m_strURI].GetRefsCount());

			m_nFilePos = 0;
			m_llResponceContentLength = m_pFile->GetFileSize();

			ostringstream str;
			str << "HTTP/1.1 200 OK\r\n"
				<< "Content-Type: " << m_pFile->GetContentType() << "\r\n"
				<< "Content-Length: " << m_llResponceContentLength << "\r\n"
				<< "\r\n";
			m_strResponceHeader = str.str();
		}
		inline void OnSending(CInterface<startupType> *pServer, int nEvent)
		{
			DEBUG_LOG("OnSending start (for uri=%s)", m_httpHeader.GetURI().c_str());
			if ((nEvent != -1) && (!(pServer->m_events[nEvent].events & EPOLLOUT)))
				return;
			else if ((nEvent == -1) && (!FD_ISSET(*m_pSocket, &pServer->m_writefds)))
			{
				if (m_llResponceContentLength == -1)
					m_CurrentAction.Set(A_CGI_CONTINUE);

				return;
			}

			if (!m_pSocket->IsAllSended())
			{
				if (!m_pSocket->ContinueSend())
				{
					DEBUG_LOG("ContinueSend error\n");
					m_CurrentAction.Set(A_ERROR);
				}
				else
					m_CurrentAction.ResetTimeout();
				return;
			}
			m_llCurrentSendLength += m_pSocket->GetSendPosition();
			if ((m_llCurrentSendLength >= m_llResponceContentLength) &&
				(m_llCurrentSendLength-m_nResponceHeaderLength >= m_llResponceContentLength) &&
				!(m_strResponceHeader.length()))
			{
				m_CurrentAction.Set(A_ALL_SENDED);
				//m_CurrentAction.ResetTimeout();
				return;
			}

			m_pSocket->InitSend();
			if (!SendResponce(pServer))
			{
				DEBUG_LOG("send responce error\n");
				m_CurrentAction.Set(A_ERROR);
			}
			
			m_CurrentAction.ResetTimeout();
		}

		inline bool OnAllSended(CInterface<startupType> *pServer)
		{
			m_uiAllSended += m_llCurrentSendLength;

			d_printf("All sended\n");
			DEBUG_LOG("All sended");
			if (m_llResponceContentLength == -1)
			{
				m_CurrentAction.Set(A_CGI_CONTINUE);
				return true;
			}
			if (!m_bKeepAlive)
				m_pSocket->ShutdownSocket(SD_SEND);

			pServer->m_StartupInfo.EraseClient(id());
			
			if (!m_bKeepAlive)
				return false;

			Cleanup();
			DEBUG_LOG("Set A_ACCEPTED for Keep-Alive");
			m_CurrentAction.Set(A_ACCEPTED);
			return true;
		}

		inline void OnStartCGI(CInterface<startupType> *pServer)
		{
			DEBUG_LOG("OnStartCGI id=%i", m_CurrentAction.id());

			vector<string> vQueryes = m_httpHeader.GetQueryes();
			pServer->m_StartupInfo.RegisterCGIClient(
				m_CurrentAction.id(), vQueryes, m_httpHeader.GetURI(),
				m_httpHeader.GetContentLength(),
				m_httpHeader.GetBoundary(),
				m_httpHeader.GetContentType(),
				m_pSocket->GetIP(),
				m_httpHeader.GetQuery(),
				m_httpHeader.GetHeadersMap(),
				m_pSocket->IsSSL());

			m_strResponceHeader = m_strURI = "";
			m_llResponceContentLength = -1;
			m_CurrentAction.Set(A_CGI_CONTINUE);
		}
	private:
		time_t m_tmLastCGIDataTime;
		bool m_bHeaderSended;
		string m_strCGIResponceHeader;
	public:
		void OnErase(CInterface<startupType> *pServer)
		{
			DEBUG_LOG("Erase id=%i", id());
			pServer->m_StartupInfo.EraseClient(id());
		}
		inline void OnCGIContinue(CInterface<startupType> *pServer)
		{
			//DEBUG_LOG("Client call OnCGIContinue id=%i", m_CurrentAction.id());

			vector<BYTE> vOut;

			vector<BYTE> vCurrentBody = m_httpHeader.GetCurrentBody();
#ifdef _DEBUG
			if (vCurrentBody.size())
				__asm nop;
#endif
			if (!pServer->m_StartupInfo.GetCGIInfo(id(), &vCurrentBody, &vOut))
			{
				DEBUG_LOG("Client return from OnCGIContinue with A_CGI_END (script error) m_bKeepAlive=%i; id=%i", m_bKeepAlive, id());
				pServer->m_StartupInfo.EraseClient(id());

				m_CurrentAction.Set(A_CGI_END);
				DEBUG_LOG("m_bKeepAlive=%i", m_bKeepAlive);
				return;
			}

			if (vOut.size())
			{
				DEBUG_LOG("CGI OUT have size=%i; id=%i", vOut.size(), id());
				m_tmLastCGIDataTime = time(NULL);

				if (!m_bHeaderSended)
				{
					DEBUG_LOG("!m_bHeaderSended");
					const string strResponce((const char *)&vOut[0], vOut.size());
					/*const string strResponce = [](const vector<BYTE> &vOut)//(const char *)&vOut[0];
						{
							string strOut;
							for (size_t n=0; n<vOut.size(); n++)
								strOut += vOut[n];
							return strOut;
						}(vOut);*/
						

					const int nEndHeader = strResponce.find("\r\n\r\n");

					if (nEndHeader == strResponce.npos)
					{
						m_CurrentAction.Set(A_CGI_END);
						DEBUG_LOG("Client return from OnCGIContinue with A_CGI_END (bad responce http header)");
						return;
					}

					const string strHeader = strResponce.substr(0, nEndHeader+2);
					m_strCGIResponceHeader = strHeader;
					const string strKeepAlive = "Connection: Keep-Alive\r\n";
					const string strClose = "Connection: Close\r\n";
					const string strKeepAlive2 = "Connection: keep-alive\r\n";
					if (m_bKeepAlive && (strHeader.find(strKeepAlive) == strHeader.npos && strHeader.find(strKeepAlive2) == strHeader.npos))
					{
						DEBUG_LOG("condition 1 start");
						const int nPos = strHeader.find("\r\n");
						if (nPos == strHeader.npos)
						{
							DEBUG_LOG("ERROR: nPos == strHeader.npos");
							//return;
						}

						DEBUG_LOG("condition 1 step0, m_ResponceBody.size()=%i", m_ResponceBody.size());
						m_ResponceBody.insert(m_ResponceBody.end(), vOut.begin(), vOut.begin()+nPos+2);
						DEBUG_LOG("condition 1 step1, strKeepAlive=%s", strKeepAlive.c_str());
						m_ResponceBody.insert(m_ResponceBody.end(), strKeepAlive.c_str(), strKeepAlive.c_str()+strKeepAlive.length());
						DEBUG_LOG("condition 1 step2, vOut.size()=%i, nPos=%i", vOut.size(), nPos);
						m_ResponceBody.insert(m_ResponceBody.end(), vOut.begin()+nPos+2, vOut.end());

						DEBUG_LOG("condition 1 step3");
						/*printf (strTemp.c_str());

						int nPos2 = strTemp.find("\r\n\r\n");
						int nLen = strTemp.length() - nPos2-4;
						__asm nop;*/
						/*m_ResponceBody.push_back(0);
						DEBUG_LOG((const char *)&m_ResponceBody[0]);
						m_ResponceBody.pop_back();*/
					}
					else
					{
						DEBUG_LOG("condition 2 start");
						const int nPos = strHeader.find("\r\n");
						if (nPos == strHeader.npos)
						{
							DEBUG_LOG("ERROR: nPos == strHeader.npos");
							//return;
						}

						DEBUG_LOG("condition 2 step0, m_ResponceBody.size()=%i", m_ResponceBody.size());
						m_ResponceBody.insert(m_ResponceBody.end(), vOut.begin(), vOut.begin()+nPos+2);
						DEBUG_LOG("condition 2 step1, strClose=%s", strClose.c_str());
						m_ResponceBody.insert(m_ResponceBody.end(), strClose.c_str(), strClose.c_str()+strClose.length());
						DEBUG_LOG("condition 2 step2, vOut.size()=%i, nPos=%i", vOut.size(), nPos);
						m_ResponceBody.insert(m_ResponceBody.end(), vOut.begin()+nPos+2, vOut.end());
						
						DEBUG_LOG("condition 2 step3");
						/*m_ResponceBody.push_back(0);
						DEBUG_LOG((const char *)&m_ResponceBody[0]);
						m_ResponceBody.pop_back();*/
					}
					m_bHeaderSended = true;
				}
				else if (m_ResponceBody.size()+vOut.size() > 0)
				{
					DEBUG_LOG("(m_ResponceBody.size()+vOut.size() > 0)");
					//m_ResponceBody.insert(m_ResponceBody.end(), vOut.begin(), vOut.end());
					vector<BYTE> temp(m_ResponceBody.size()+vOut.size());

					if (m_ResponceBody.size())
						memcpy(&temp[0], &m_ResponceBody[0], m_ResponceBody.size());
					if (vOut.size())
						memcpy(&temp[m_ResponceBody.size()], &vOut[0], vOut.size());

					m_ResponceBody = move(temp);
				}

				m_CurrentAction.Set(A_SENDING);
				if (m_httpHeader.GetMethod() == "POST")
				{
					//m_llResponceContentLength = m_ResponceBody.size();
				}


			}
			else
			{
				//DEBUG_LOG("vOut.size() == 0");
				if (m_httpHeader.GetMethod() == "POST")
				{
					DEBUG_LOG("POST METHOD id=%i BodyLength=%llu, ContentLength=%llu", m_CurrentAction.id(), m_httpHeader.GetBodyLength(), m_httpHeader.GetContentLength());
					if (m_httpHeader.GetBodyLength() < m_httpHeader.GetContentLength())
						StartReading();
					else
					{
						/*Cleanup();
						DEBUG_LOG("POST request all sent");
						m_CurrentAction.Set(A_ACCEPTED);*/
						//return;
						if (m_strCGIResponceHeader.find("HTTP/1.1 100 ") == 0)
							m_bHeaderSended = false;
					}
				}
			}

			if (!m_tmLastCGIDataTime)
				m_tmLastCGIDataTime = time(NULL);
#ifndef _DEBUG
			if (time(NULL) - m_tmLastCGIDataTime > 30)
#else
			if (time(NULL) - m_tmLastCGIDataTime > 150)
#endif
			{
				pServer->m_StartupInfo.EraseClient(id());

				m_CurrentAction.Set(A_CGI_END);
				DEBUG_LOG("Client return from OnCGIContinue with A_CGI_END (time out); id=%i", id());
				return;
			}

			//DEBUG_LOG("Client return from OnCGIContinue with %i", m_CurrentAction.Get());
		}
		inline bool SendHeader()
		{
			if (!m_strResponceHeader.length())
				return false;

			if (!m_pSocket->SendBuffer((const char*)m_strResponceHeader.c_str(), m_strResponceHeader.length()))
			{
				d2_printf("error: (!m_Socket.SendBuffer)\n");
				m_CurrentAction.Set(A_ERROR);
			}

			m_nResponceHeaderLength = m_strResponceHeader.length();
			m_strResponceHeader = "";
			m_bHeaderSended = true;
			return true;
		}

		inline bool SendResponce(CInterface<startupType> *pServer)
		{
			d_printf("SendResponce\n");
			DEBUG_LOG("Try SendResponce");
			if ((m_pFile) && (!m_pFile->IsOpen()) && (m_strURI.size()))
			{
				d_printf("NOT FOUND uri=%s\n", m_strURI.c_str());
				DEBUG_LOG("NOT FOUND uri=%s", m_strURI.c_str());

				m_strResponceHeader = "HTTP/1.1 404 not found\r\n\r\n";

				if (!GetError(pServer, "404.html"))
				{
					DEBUG_LOG("SendResponce return false");
					return false;
				}
			}

			DEBUG_LOG("SendResponce check responce length");
			if (m_strResponceHeader.length())
				return SendHeader();

			m_CurrentAction.Set(A_ALL_SENDED);
			if ((m_pFile) && (m_pFile->IsOpen()))
				m_CurrentAction.Set(m_pSocket->SendFile(m_pFile, &m_nFilePos));
			else
			{
				m_CurrentAction.Set(m_pSocket->SendVector(&m_ResponceBody));
				m_ResponceBody.clear();
			}

			DEBUG_LOG("SendResponce return with action=%i", m_CurrentAction.Get());
			return true;
		}

		const bool IsMethodPost() const {return m_bIsMethodPost;}
		const uint64_t GetSendCount() const {return m_uiAllSended;}
		const string GetIP() const {return m_pSocket->GetIP();}
		const string GetURI() const {return m_strLastURI;}


		inline bool GetError(CInterface<startupType> *pServer, string strError)
		{
			if (!pServer->m_StartupInfo.GetErrors().length())
			{
				d_printf("error pages path not set\n");
				return false;
			}
			string strPath = pServer->m_StartupInfo.GetErrors();
			strPath += "\\";
			strPath += strError;

			m_strURI = strPath;

			m_pFile = shared_ptr<CFile>( new CFile(m_strURI, m_bGzip));
			return m_pFile->IsOpen();
		}
	};

	class CBlack
	{
		int m_nCount, m_nAccepts, m_nPosts;
		uint64_t m_uiTraffic;
		time_t m_tmTime;
	public:
		CBlack(const int nCount) : m_nCount(nCount), m_tmTime(time(0)), m_nAccepts(0), m_nPosts(0), m_uiTraffic(0) {}
		const bool IsTimeout() const {return time(0) - m_tmTime > 500;}
		const int Count() const {return m_nCount;}
		void SetCount(const int nCount) {m_nCount = nCount;}

		void UpdateTraffic(uint64_t uiTraffic) 
		{
			m_uiTraffic += uiTraffic;
		}

		void OnPost() {m_nPosts++;}
		const int GetPostSpeed() 
		{
			const time_t tmCurrent = time(0);
			
			DEBUG_LOG("GetPostSpeed tmCurrent - m_tmTime = %i; m_nPosts=%i", tmCurrent - m_tmTime, m_nPosts);
			if (tmCurrent <= m_tmTime) return 1+60*m_nPosts;
			return 60*m_nPosts/(tmCurrent - m_tmTime);
		}
		void OnAccept() {m_nAccepts++; m_nCount--;}
		const int GetSpeed() const 
		{
			const time_t tmCurrent = time(0);

			DEBUG_LOG("GetSpeed tmCurrent - m_tmTime = %i; m_nAccepts=%i", tmCurrent - m_tmTime, m_nAccepts);
			if (tmCurrent <= m_tmTime) return 1+m_nAccepts;
			return m_nAccepts/(tmCurrent - m_tmTime);
		}
		const int GetAccepts() const {return m_nAccepts;}
		const uint64_t GetTraffic() 
		{
			return m_uiTraffic;
		}
	};

	template <class startupType>
	class CServer
	{
		unordered_map<SOCKET, shared_ptr<CClient<startupType> > > m_Clients;
		unordered_map<string, shared_ptr<CBlack>> m_mapBlacklistIP;

		vector<string> m_vstrWhiltLestIP;
	private:
		struct epoll_event m_ListenEvent, m_ListenEventSSL;
		int m_epoll;

	public:

		bool m_bIsInit;
		CServer(startupType StartupInfo) :
		  m_Interface(StartupInfo), m_epoll(-1), m_bIsInit(false)
		{
#ifdef MEM_DEBUG
			return;
#endif
			cout << "Server Created";

			BEGIN_SEND;
#ifdef _WIN32
			WSADATA wsaData;
			if ( WSAStartup( MAKEWORD( 2, 2 ), &wsaData ) != 0 )
			{
				/* Tell the user that we could not find a usable */
				/* WinSock DLL.                                  */
				cout << "Could not to find usable WinSock in WSAStartup\n";
				return;
			}
#endif
#ifdef _EPOLL
			m_epoll = epoll_create (1);
			if (m_epoll == -1)
			{
				cout << "error: epoll_create\n";
				return;
			}
#endif
			SSL_load_error_strings();
			SSLeay_add_ssl_algorithms();

			m_pListenSocket = shared_ptr<CSocket>(new CSocket(StartupInfo.GetPort(), false));
			if ((SOCKET)*m_pListenSocket == INVALID_SOCKET)
			{
				cout << "Error creating Listen Socket\n";
				return;
			}

//#ifndef _WIN32
			m_pListenSocketSSL = shared_ptr<CSocket>(new CSocket(StartupInfo.GetPortSSL(), true));
			if ((SOCKET)*m_pListenSocketSSL == INVALID_SOCKET)
			{
				cout << "Error creating Listen Socket\n";
				return;
			}
//#endif
#ifdef _EPOLL
			m_ListenEvent.data.fd = (SOCKET)(*m_pListenSocket);
			m_ListenEvent.events = EPOLLIN;// | EPOLLET;
			epoll_ctl (m_epoll, EPOLL_CTL_ADD, (SOCKET)(*m_pListenSocket), &m_ListenEvent);
			m_nSocketsCount = 1;

			m_ListenEventSSL.data.fd = (SOCKET)(*m_pListenSocketSSL);
			m_ListenEventSSL.events = EPOLLIN;// | EPOLLET;
			epoll_ctl (m_epoll, EPOLL_CTL_ADD, (SOCKET)(*m_pListenSocketSSL), &m_ListenEventSSL);
			m_nSocketsCount = 2;
#endif
			m_bIsInit = true;
		}

		inline void SetSockets()
		{
			FD_ZERO(&m_Interface.m_readfds);
			FD_ZERO(&m_Interface.m_writefds);
			FD_ZERO(&m_Interface.m_exceptfds);

			m_nFDS = 0;

			for (auto it = m_Clients.begin(); it != m_Clients.end(); ++it)
			{
				SOCKET hSocket = it->first;
				if (hSocket == INVALID_SOCKET)
					continue;

				if ((int)hSocket > m_nFDS)
					m_nFDS = (int)hSocket;

				FD_SET(hSocket, &m_Interface.m_readfds);
				FD_SET(hSocket, &m_Interface.m_writefds);
				FD_SET(hSocket, &m_Interface.m_exceptfds);
			}
		}

		inline void AcceptClients(const shared_ptr<CSocket> pSocket)
		{
			for (int n=0; n<1000; n++)
			{
				struct sockaddr_in6	server;
				const SOCKET hSocket = pSocket->Accept(&server);

				if (hSocket == INVALID_SOCKET)
					return;

				auto it = m_Clients.find(hSocket);
				if (it != m_Clients.end())
				{
					DEBUG_LOG("Close client from AcceptClients");
					CloseClient(it->first, it->second);
				}

				const string strIP = [](struct in6_addr Sin6Addr) -> string
				{
					char saddr[INET6_ADDRSTRLEN];

#ifndef _WIN32
					inet_ntop(AF_INET6, (void *)&Sin6Addr, saddr, INET6_ADDRSTRLEN);
#else
					DWORD len = INET6_ADDRSTRLEN;
					WSAAddressToString((LPSOCKADDR)(&Sin6Addr), sizeof(Sin6Addr), NULL, saddr, &len);
#endif

					return string(saddr);
				}(server.sin6_addr);

				DEBUG_LOG("Accepted ip=%s", strIP.c_str() );

				UpdateBlacklist(strIP);

				const bool bInWhiteList = InWhiteList(strIP);

				auto it2 = m_mapBlacklistIP.find(strIP);
				if (it2 != m_mapBlacklistIP.end())
				{
					it2->second->OnAccept();

					int nMaxSpeed = 500;

					DEBUG_LOG("nSocketsCount=%i  nMaxSpeed = %i; Speed=%i; PostSpeed=%i", m_nSocketsCount, nMaxSpeed, it2->second->GetSpeed(), it2->second->GetPostSpeed());
					if (((it2->second->GetSpeed() > nMaxSpeed) && (!bInWhiteList)) ||
						(it2->second->GetPostSpeed() > 2) ||
						((it2->second->GetTraffic() > 500000000) && (!bInWhiteList)))
					{
						DEBUG_LOG("Close blacklisted ip=%s; Speed=%i; PostSpeed=%i", strIP.c_str(), it2->second->GetSpeed(), it2->second->GetPostSpeed());
						shutdown(hSocket, SD_BOTH);
						closesocket(hSocket);
						continue;
					}
					if (bInWhiteList ||
						((it2->second->GetSpeed() == 0) && (it2->second->GetPostSpeed() == 0) && (m_nSocketsCount < 200)))
					{
						DEBUG_LOG("m_mapBlacklistIP.erase(%s)", strIP.c_str());
						m_mapBlacklistIP.erase(strIP);
					}
				}
				it2 = m_mapBlacklistIP.find(strIP);
				if (it2 != m_mapBlacklistIP.end())
				{
					DEBUG_LOG("Check blacklist ip=%s; count=%i; Speed=%i; PostSpeed=%i", strIP.c_str(), it2->second->Count(), it2->second->GetSpeed(), it2->second->GetPostSpeed());

					int nMaxCount = 10;
					if (m_nSocketsCount > 200)
						nMaxCount = 5;
					if (m_nSocketsCount > 400)
						nMaxCount = 2;

					if ((it2->second->Count()) > nMaxCount && (!bInWhiteList))
					{
						DEBUG_LOG("Close blacklisted ip=%s; Count=%i; PostSpeed=%i", strIP.c_str(), it2->second->Count(), it2->second->GetPostSpeed());
						shutdown(hSocket, SD_BOTH);
						closesocket(hSocket);
						continue;
					}
				}
				DEBUG_LOG("Check blacklist failed ip=%s", strIP.c_str());
				


				m_Clients[hSocket] = shared_ptr<CClient<startupType> >(new CClient<startupType>(hSocket, &server, pSocket->IsSSL()));

				it = m_Clients.find(hSocket);

#ifdef _EPOLL
				it->second->m_Event.data.fd = it->first;
				it->second->m_Event.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLOUT;
				epoll_ctl (m_epoll, EPOLL_CTL_ADD, it->first, &it->second->m_Event);
				m_nSocketsCount++;
#endif
				//d_printf("Accepted\n");
				DEBUG_LOG("Accepted new client. All Clients count = %i", m_Clients.size());
			}
		}

		unordered_map<string, time_t> m_mapURI_to_Time;
		void CloseClient(const SOCKET hSocket, shared_ptr<CClient<startupType> > pClient)
		{
	
			const string strIP = pClient->GetIP();
			const string strURI = pClient->GetURI();
			
			DEBUG_LOG("Close client id=%i; ip=%s; sendcount=%i; uri=%s\n", pClient->id(), strIP.c_str(), pClient->GetSendCount(), strURI.c_str());
			auto it = m_mapBlacklistIP.find(strIP);
			if (it != m_mapBlacklistIP.end())
				it->second->UpdateTraffic(pClient->GetSendCount());

			if (pClient->IsMethodPost())
				UpdateBlacklistOnMethodPost(strIP);

			
			if ((pClient->GetSendCount() == 0) || (!strURI.length()))
			{
				//UpdateBlacklist(strIP); //update blacklist if sent no data 
			}
			else
			{
				bool bFirstTime = false;
				auto it = m_mapURI_to_Time.find(strIP+"/"+strURI);
				if (it == m_mapURI_to_Time.end())
				{
					m_mapURI_to_Time[strIP+"/"+strURI] = time(0);
					bFirstTime = true;
				}

				it = m_mapURI_to_Time.find(strIP+"/"+strURI);
				if ((it != m_mapURI_to_Time.end()) && (time(0) - it->second <= 3) && (!bFirstTime))
				{
					UpdateBlacklist(strIP); //update blacklist if less 3 sec between same URI request

					DEBUG_LOG("WARNING: ip=%s; seems DDOS to %s", strIP.c_str(), strURI.c_str());
				}
				else if (it != m_mapURI_to_Time.end())
				{
					m_mapURI_to_Time.erase(strIP+"/"+strURI);

					auto it = m_mapBlacklistIP.find(strIP);
					if (it != m_mapBlacklistIP.end())
					{
						if (it->second->Count() < 100)
						{
							//m_mapBlacklistIP.erase(strIP);
						}
						else
							it->second->SetCount(it->second->Count() - 10);
					}
				}
			}


#ifdef _EPOLL
			epoll_ctl (m_epoll, EPOLL_CTL_DEL, hSocket, NULL);
#endif
			m_nSocketsCount--;
			pClient->OnErase(&m_Interface);
			m_Clients.erase(hSocket);
		}

		void UpdateBlacklistOnMethodPost(const string strIP)
		{
			auto it = m_mapBlacklistIP.find(strIP);
			if (it == m_mapBlacklistIP.end())
			{
				UpdateBlacklist(strIP);
				it = m_mapBlacklistIP.find(strIP);
			}

			if (it != m_mapBlacklistIP.end())
				it->second->OnPost();
		}

		bool InWhiteList(const string strIP) const
		{
			if (g_vWhiteListedIP.find(strIP) != g_vWhiteListedIP.end())
			{
					DEBUG_LOG("White List ip=%s", strIP.c_str());
					return true;
			}
			for (size_t n=0; n<m_vstrWhiltLestIP.size(); n++)
			{
				if (strIP.find(m_vstrWhiltLestIP[n]) == 0)
				{
					DEBUG_LOG("White List (Cloudflare) ip=%s", strIP.c_str());
					return true;
				}
			}
			return false;
		}

		void UpdateBlacklist(const string strIP)
		{
			//return;
			static time_t tmLastTime = time(0);
			time_t tmCurrent = time(0);

			if (tmCurrent - tmLastTime > 1000)
			{
				tmLastTime = tmCurrent;
				m_mapBlacklistIP.clear();
				g_vWhiteListedIP.clear();
			}

			auto it = m_mapBlacklistIP.find(strIP);
			if (it == m_mapBlacklistIP.end())
			{
				DEBUG_LOG("Add to blacklist ip=%s", strIP.c_str());
				m_mapBlacklistIP[strIP] = shared_ptr<CBlack>(new CBlack(0));
				it = m_mapBlacklistIP.find(strIP);
			}

			if (it->second->IsTimeout() && it->second->Count() > 10 && (m_nSocketsCount < 200))
			{
				if (it->second->Count() < 1000)
				{
					DEBUG_LOG("m_mapBlacklistIP.erase(%s)", strIP.c_str());
					m_mapBlacklistIP.erase(strIP);
				}
				return;
			}

			DEBUG_LOG("Increase blacklist ip=%s; value=%i", strIP.c_str(), it->second->Count() + 1);
			it->second->SetCount(it->second->Count() + 1);
		}

		void CloseTimeouts()
		{
			vector<SOCKET> closedSockets;

			for (auto it = m_Clients.begin(); it != m_Clients.end(); ++it)
			{
				if (it->second->IsTimeout())
				{
					DEBUG_LOG("Client Timeout");
					it->second->OnErase(&m_Interface);

					closedSockets.push_back(it->first);
				}
			}
			for (size_t n=0; n<closedSockets.size(); n++)
			{
				m_nSocketsCount--;
				m_Clients.erase(closedSockets[n]);
			}
		}

		void CallbackClient(typename unordered_map<SOCKET, shared_ptr<CClient<startupType> > >::iterator &it, const int nEvent = -1)
		{
			if (!it->second->Callback(&m_Interface, nEvent))
			{
				DEBUG_LOG("Client callback return false - Erase client");
				//it->second->OnErase(&m_Interface);
				//m_Clients.erase(it->first);
				CloseClient(it->first, it->second);
			}
		}

		int GetSpeed(shared_ptr<CClient<startupType> > pClient) const
		{
			auto it = m_mapBlacklistIP.find(pClient->GetIP());
			if (it == m_mapBlacklistIP.end())
				return 0;

			return it->second->GetSpeed();
		}

		inline void Continue()
		{
			m_Interface.m_StartupInfo.ContinuePlugins();

			static time_t tm = time(0);
			time_t tmCurr = time(0);
			if (tmCurr - tm > 5)
			{
				tm = tmCurr;
				printf("Clients = %i\n",(int)m_nSocketsCount);
				DEBUG_LOG("Clients = %i\n",(int)m_nSocketsCount);

				CloseTimeouts();
			}
			Sleep(5);
#ifdef _EPOLL
			if (m_nSocketsCount == 2)
				m_Clients.clear();

			m_Interface.m_events.resize(m_nSocketsCount);
			int n = epoll_wait (m_epoll, &m_Interface.m_events[0], m_nSocketsCount, 5000);

			DEBUG_LOG("epoll continue Clients = %i; blacklist=%i\n",m_nSocketsCount, m_mapBlacklistIP.size());

			for (int i = 0; i < n; i++)
			{
				if (n > 100)
					usleep(1);

				AcceptClients(m_pListenSocket);
				AcceptClients(m_pListenSocketSSL);

				SOCKET hSocketIn = m_Interface.m_events[i].data.fd;

				if ((SOCKET)(*m_pListenSocket) == hSocketIn)
					continue;
				if ((SOCKET)(*m_pListenSocketSSL) == hSocketIn)
					continue;

				auto it = m_Clients.find(hSocketIn);
				if (it == m_Clients.end())
				{
					printf("fatal error: socket not found!\n");
					DEBUG_LOG("fatal error: socket not found!");
					close (hSocketIn);
					continue;
				}

				if (!it->second->Callback(&m_Interface, i))
				{
					DEBUG_LOG("Client callback return false - Erase client");

					CloseClient(it->first, it->second);
				}
			}
			return;
#else
			AcceptClients(m_pListenSocket);

			if (m_pListenSocketSSL)
				AcceptClients(m_pListenSocketSSL);

			SetSockets();

			struct timeval tv;
			tv.tv_sec = 5;
			tv.tv_usec = 0;
			m_nFDS++;
			select(m_nFDS, &m_Interface.m_readfds, &m_Interface.m_writefds, &m_Interface.m_exceptfds, &tv);

			auto it = m_Clients.begin();
			while (m_Clients.size() && it != m_Clients.end()) //Ïåðå÷èñëÿåì âñåõ êëèåíòîâ
			{
				Sleep(5);
				if ((GetSpeed(it->second) > 1) && (!InWhiteList(it->second->GetIP())))
					continue;
				CallbackClient(it++);
			}
#endif
		}
	private:
		shared_ptr<CSocket> m_pListenSocket, m_pListenSocketSSL;
		int m_nFDS;

		CInterface<startupType> m_Interface;

		size_t m_nSocketsCount;
	};
}

#endif //_SIMPLE_SERVER
