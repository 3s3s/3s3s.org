#ifndef _SITE_UTILS
#define _SITE_UTILS
#include "orm.h"
#include "../html_framework.h"

namespace site
{
	class utils
	{
	public:
		static const string GetToken(const string strUID)
		{
			static time_t tmDiff = 0;

			return
				html::utils::md5(strUID) + 
				html::utils::md5(orm::utils::TimeToString(time(NULL)+tmDiff++));
		}
	};
}
#endif