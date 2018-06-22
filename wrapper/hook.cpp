#include "hook.h"

void EnsureMinHookInitialized() {
	// Ensure MH_Initialize is only called once.
	static LONG minhook_init_once = 0;
	if (InterlockedCompareExchange(&minhook_init_once, 1, 0) == 0) {
		MH_STATUS status = MH_Initialize();
		if (status != MH_OK && status != MH_ERROR_ALREADY_INITIALIZED)
			LOG(WARN) << "Failed to initialize minhook; MH_Initialize returned" << MH_StatusToString(status);
	}
}
