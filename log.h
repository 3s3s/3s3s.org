#define DEFAULT_DNS_NAME	"3s3s.ru"

#define WWW_ROOT	"wwwroot"

#define DATABASE	"Dictionary.db"

#define CERTF  "cert.pem"
#define KEYF   CERTF

#define USER_AGENT_3S3S		"user_agent_3s3s"
#define PROXY_3S3S			"add_proxy_3s3s"

#define _DEBUG_LOG

//#define DEBUG_LOG //

#ifdef _DEBUG_LOG
#define DEBUG_LOG debug_log
void debug_log(char const* fmt, ...); 
#else
#define DEBUG_LOG //
#endif

#include <string>
#include <vector>
extern std::string DNS_NAME;
extern std::vector<std::string> g_vRedirectOFF;

bool IsRedirectOn(const std::string strURL);