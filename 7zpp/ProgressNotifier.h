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
			virtual bool ShouldCancel()
			{
				return false;
			}

			// Called at beginning
			virtual void OnStart(TStringView status, int64_t bytesTotal) {}

			// Called whenever progress has updated with a bytes complete
			virtual void OnProgress(TStringView status, int64_t bytesCompleted) {}

			// Called when progress has reached 100%
			virtual void OnEnd() {}
	};

	class ProgressNotifierDelegate final
	{
		private:
			ProgressNotifier* m_Notifier = nullptr;

		public:
			ProgressNotifierDelegate(ProgressNotifier* notifier = nullptr)
				:m_Notifier(notifier)
			{
			}

		public:
			ProgressNotifier* GetNotifier() const
			{
				return m_Notifier;
			}
			void SetNotifier(ProgressNotifier* notifier)
			{
				m_Notifier = notifier;
			}

			bool ShouldCancel()
			{
				if (m_Notifier)
				{
					m_Notifier->ShouldCancel();
				}
				return false;
			}
			void OnStart(TStringView status, int64_t bytesTotal)
			{
				if (m_Notifier)
				{
					m_Notifier->OnStart(status, bytesTotal);
				}
			}
			void OnProgress(TStringView status, int64_t bytesCompleted)
			{
				if (m_Notifier)
				{
					m_Notifier->OnProgress(status, bytesCompleted);
				}
			}
			void OnEnd()
			{
				if (m_Notifier)
				{
					m_Notifier->OnEnd();
				}
			}
			
		public:
			ProgressNotifierDelegate& operator=(ProgressNotifier* notifier)
			{
				SetNotifier(notifier);
				return *this;
			}
			
			operator ProgressNotifier* () const
			{
				return GetNotifier();
			}
			ProgressNotifier* operator*() const
			{
				return GetNotifier();
			}
	};
}
