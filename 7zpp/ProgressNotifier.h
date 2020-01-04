#pragma once
#include "SevenZipLibrary.h"
#include "SevenString.h"

namespace SevenZip
{
	class ProgressNotifier
	{
		public:
			virtual ~ProgressNotifier() = default;

		public:
			// Called whenever operation can be stopped. Return true to abort operation.
			virtual bool ShouldStop()
			{
				return false;
			}

			// Called at beginning
			virtual void OnStartWithTotal(TStringView filePath, int64_t totalBytes) {}

			// Called whenever progress has updated with a bytes complete
			virtual void OnMajorProgress(TStringView filePath, int64_t bytesCompleted) {}

			// Called when single file progress has reached 100%, returns the file path that completed
			virtual void OnMinorProgress(TStringView filePath, int64_t bytesCompleted, int64_t totalBytes) {}

			// Called when progress has reached 100%
			virtual void OnDone(TStringView filePath = {}) {}
	};
}
