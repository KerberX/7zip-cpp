#pragma once
#include "SevenZipLibrary.h"
#include "CompressionFormat.h"

namespace SevenZip
{
	class ProgressNotifier
	{
		public:
			// Called whenever operation can be stopped. Return true to abort operation.
			virtual bool Stop()
			{
				return false;
			}

			// Called at beginning
			virtual void OnStartWithTotal(const TCharType* filePath, int64_t totalBytes) {}

			// Called whenever progress has updated with a bytes complete
			virtual void OnMajorProgress(const TCharType* filePath, int64_t bytesCompleted) {}

			// Called when single file progress has reached 100%, returns the filepath that completed
			virtual void OnMinorProgress(const TCharType* filePath, int64_t bytesCompleted, int64_t totalBytes) {}

			// Called when progress has reached 100%
			virtual void OnDone(const TCharType* filePath = _T("")) {}

		public:
			void OnStartWithTotal(const TString& filePath, int64_t totalBytes)
			{
				OnStartWithTotal(filePath.c_str(), totalBytes);
			}
			void OnMajorProgress(const TString& filePath, int64_t bytesCompleted)
			{
				OnMajorProgress(filePath.c_str(), bytesCompleted);
			}
			void OnMinorProgress(const TString& filePath, int64_t bytesCompleted, int64_t totalBytes)
			{
				OnMinorProgress(filePath.c_str(), bytesCompleted, totalBytes);
			}
			void OnDone(const TString& filePath)
			{
				OnDone(filePath.c_str());
			}
	};
}
