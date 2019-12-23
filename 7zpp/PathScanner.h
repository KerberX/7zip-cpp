#pragma once
#include <deque>
#include "FileInfo.h"

namespace SevenZip::PathScanner
{
	class Callback
	{
		public:
			virtual ~Callback() = default;

		public:
			virtual void BeginScan() {}
			virtual void EndScan() {}

			virtual bool ShouldDescend(const FilePathInfo& directory) = 0;
			virtual void EnterDirectory(const TString& path) {}
			virtual void LeaveDirectory(const TString& path) {}

			virtual void ExamineFile(const FilePathInfo& file, bool& exit) = 0;
	};
}

namespace SevenZip::PathScanner
{
	void Scan(const TString& root, Callback& callback);
	void Scan(const TString& root, const TString& searchPattern, Callback& callback);
}

