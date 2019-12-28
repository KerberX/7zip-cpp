#pragma once

#define SEVENZIP_VERSION L"2019-12-23.0"
#define SEVENZIP_BRANCH L"master"

#ifdef _DEBUG
	#ifdef _UNICODE
		#pragma comment( ib, "7zpp_ud.lib")
	#else
		#pragma comment(lib, "7zpp_ad.lib")
	#endif
#else
	#ifdef _UNICODE
		#pragma comment(lib, "7zpp_u.lib")
	#else
		#pragma comment(lib, "7zpp_a.lib")
	#endif
#endif
