#pragma once
#include "../../7zpp/SevenZipLibrary.h"
#include "../../7zpp/SevenZipArchive.h"
#include "../../7zpp/ProgressNotifier.h"
#include "../../7zpp/ListingNotifier.h"

// Version of this library
#define SEVENZIP_VERSION L"20160116.1"
#define SEVENZIP_BRANCH L"master"

#ifdef _DEBUG
#ifdef _UNICODE
#pragma comment ( lib, "7zpp_ud.lib" )
#else
#pragma comment ( lib, "7zpp_ad.lib" )
#endif
#else
#ifdef _UNICODE
#pragma comment ( lib, "7zpp_u.lib" )
#else
#pragma comment ( lib, "7zpp_a.lib" )
#endif
#endif

