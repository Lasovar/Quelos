#include <qspch.h>
#include <unordered_set>

#include "Ref.h"

namespace Quelos {
	namespace RefUtils {

		static std::unordered_set<void*> s_LiveReferences;
		static std::mutex s_LiveReferenceMutex;

		void AddToLiveReferences(void* instance) {
			std::scoped_lock<std::mutex> lock(s_LiveReferenceMutex);
			QS_CORE_ASSERT(instance);
			s_LiveReferences.insert(instance);
		}

		void RemoveFromLiveReferences(void* instance) {
			std::scoped_lock<std::mutex> lock(s_LiveReferenceMutex);
			QS_CORE_ASSERT(instance);
			QS_CORE_ASSERT(s_LiveReferences.find(instance) != s_LiveReferences.end());
			s_LiveReferences.erase(instance);
		}

		bool IsLive(void* instance) {
			QS_CORE_ASSERT(instance);
			return s_LiveReferences.find(instance) != s_LiveReferences.end();
		}

	}
}

