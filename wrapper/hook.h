#pragma once
#include "util.h"
#include "MinHook.h"

void EnsureMinHookInitialized();

template<typename T, typename... ARGS>
struct Hook {
	typedef T(WINAPI *FT)(ARGS...);
	FT original_, func_;
	Hook() :original_(0), func_(0) {
		EnsureMinHookInitialized();
	}
	void setup(FT f, FT n) {
		func_ = f;
		MH_STATUS status = MH_CreateHook((LPVOID)func_, (LPVOID)n, (LPVOID *)&original_);
		if (status != MH_OK)
			LOG(WARN) << "Failed to setup minhook " << MH_StatusToString(status);
		this->enable();
	}
	void enable() {
		MH_STATUS status = MH_EnableHook((LPVOID)func_);
		if (status != MH_OK)
			LOG(WARN) << "Failed to enable minhook " << MH_StatusToString(status);
	}
	void disable() {
		MH_STATUS status = MH_DisableHook((LPVOID)func_);
		if (status != MH_OK)
			LOG(WARN) << "Failed to enable minhook " << MH_StatusToString(status);
	}
	T operator()(ARGS... args) {
		return original_(args...);
	}
	~Hook() {
		MH_STATUS status = MH_RemoveHook((LPVOID)func_);
		//if (status != MH_OK)
			//LOG(WARN) << "Failed to remove minhook " << MH_StatusToString(status);
	}
};
