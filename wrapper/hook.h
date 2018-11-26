#pragma once
#include "util.h"
#include "MinHook.h"
#include <unordered_map>

void EnsureMinHookInitialized();

#pragma once

#define VTOffset(C) \
G(C,0) G(C,1) G(C,2) G(C,3) G(C,4) G(C,5) G(C,6) G(C,7) G(C,8) G(C,9) G(C,10) G(C,11) G(C,12) G(C,13) G(C,14) G(C,15) G(C,16) G(C,17) G(C,18) G(C,19) G(C,20) G(C,21) G(C,22) G(C,23) G(C,24) G(C,25) G(C,26) G(C,27) G(C,28) G(C,29) \
G(C,30) G(C,31) G(C,32) G(C,33) G(C,34) G(C,35) G(C,36) G(C,37) G(C,38) G(C,39) G(C,40) G(C,41) G(C,42) G(C,43) G(C,44) G(C,45) G(C,46) G(C,47) G(C,48) G(C,49) G(C,50) G(C,51) G(C,52) G(C,53) G(C,54) G(C,55) G(C,56) G(C,57) G(C,58) G(C,59) \
G(C,60) G(C,61) G(C,62) G(C,63) G(C,64) G(C,65) G(C,66) G(C,67) G(C,68) G(C,69) G(C,70) G(C,71) G(C,72) G(C,73) G(C,74) G(C,75) G(C,76) G(C,77) G(C,78) G(C,79) G(C,80) G(C,81) G(C,82) G(C,83) G(C,84) G(C,85) G(C,86) G(C,87) G(C,88) G(C,89) \
G(C,90) G(C,91) G(C,92) G(C,93) G(C,94) G(C,95) G(C,96) G(C,97) G(C,98) G(C,99) G(C,100) G(C,101) G(C,102) G(C,103) G(C,104) G(C,105) G(C,106) G(C,107) G(C,108) G(C,109) G(C,110) G(C,111) G(C,112) G(C,113) G(C,114) G(C,115) G(C,116) G(C,117) G(C,118) G(C,119) \
G(C,120) G(C,121) G(C,122) G(C,123) G(C,124) G(C,125) G(C,126) G(C,127) G(C,128) G(C,129) G(C,130) G(C,131) G(C,132) G(C,133) G(C,134) G(C,135) G(C,136) G(C,137) G(C,138) G(C,139) G(C,140) G(C,141) G(C,142) G(C,143) G(C,144) G(C,145) G(C,146) G(C,147) G(C,148) G(C,149)

#define G(C, n) virtual int C get ## n() { return n; }

#ifndef _M_X64
template <typename TRet, typename TObject, typename ...TArgs>
int vTableIndex(TRet(__stdcall TObject::*f)(TArgs...))
{
	struct VTableCounter
	{
		VTOffset(__stdcall);
	} vt;

	using VTMethod = int(__stdcall VTableCounter::*)();
	VTMethod getIndex = (VTMethod)f;
	return (vt.*getIndex)();
}

template <typename TRet, typename TObject, typename ...TArgs>
int vTableIndex(TRet(__cdecl TObject::*f)(TArgs...))
{
	struct VTableCounter
	{
		VTOffset(__cdecl);
	} vt;

	using VTMethod = int(__cdecl VTableCounter::*)();
	VTMethod getIndex = (VTMethod)f;
	return (vt.*getIndex)();
}

template <typename TRet, typename TObject, typename ...TArgs>
int vTableIndex(TRet(__thiscall TObject::*f)(TArgs...))
{
	struct VTableCounter
	{
		VTOffset(__thiscall);
	} vt;

	using VTMethod = int(__thiscall VTableCounter::*)();
	VTMethod getIndex = (VTMethod)f;
	return (vt.*getIndex)();
}
#endif

template <typename TRet, typename TObject, typename ...TArgs>
int vTableIndex(TRet(__fastcall TObject::*f)(TArgs...))
{
	struct VTableCounter
	{
		VTOffset(__fastcall);
	} vt;

	using VTMethod = int(__fastcall VTableCounter::*)();
	VTMethod getIndex = (VTMethod)f;
	return (vt.*getIndex)();
}


template<typename T, typename... ARGS>
struct Hook {
	typedef T(WINAPI *FT)(ARGS...);
	FT trampoline_, original_;
	Hook() :original_(0) {
		EnsureMinHookInitialized();
	}
	void setup(FT f, FT n) {
		trampoline_ = f;
		MH_STATUS status = MH_CreateHook((LPVOID)trampoline_, (LPVOID)n, (LPVOID*)&original_);
		if (status != MH_OK)
			LOG(WARN) << "Failed to setup minhook " << MH_StatusToString(status);
		this->enable();
	}
	void enable() {
		MH_STATUS status = MH_EnableHook((LPVOID)trampoline_);
		if (status != MH_OK)
			LOG(WARN) << "Failed to enable minhook " << MH_StatusToString(status);
	}
	void disable() {
		MH_STATUS status = MH_DisableHook((LPVOID)trampoline_);
		if (status != MH_OK)
			LOG(WARN) << "Failed to enable minhook " << MH_StatusToString(status);
	}
	T operator()(ARGS... args) {
		return original_(args...);
	}
	~Hook() {
		MH_STATUS status = MH_RemoveHook((LPVOID)trampoline_);
		//if (status != MH_OK)
			//LOG(WARN) << "Failed to remove minhook " << MH_StatusToString(status);
	}
};

template<typename O, typename FT>
static LPVOID implementedVirtualFunction(O * o, FT f) {
	int vti = vTableIndex(f);
	size_t *vtbl = (size_t*)*(size_t*)o;
	return (LPVOID)vtbl[vti];
}

// A hook for virtual functions
template<typename O>
struct VHook {
	struct VHookInfo {
		LPVOID original_;
		LPVOID override_;
	};
	std::unordered_map<LPVOID, VHookInfo> info_;
	VHook() {
		EnsureMinHookInitialized();
	}

template<typename T, typename... ARGS>
	void setup(O * o, T (O::*f)(ARGS...), T (*n)(O*, ARGS...)) {
		// Get the handle of the virtual function we're going to override
		LPVOID ivf = implementedVirtualFunction(o, f);

		auto i = info_.find(ivf);
		if (i != info_.end()) {
			if (i->second.override_ != n)
				LOG(WARN) << "Overriding virutal functions with different targets, this will not end well!";
			return;
		}
		
		LPVOID original;
		MH_STATUS status = MH_CreateHook(ivf, (LPVOID)n, &original);
		if (status != MH_OK)
			LOG(WARN) << "Failed to setup minhook " << MH_StatusToString(status);

		info_[ivf] = { original, n };

		status = MH_EnableHook((LPVOID)ivf);
		if (status != MH_OK)
			LOG(WARN) << "Failed to enable minhook " << MH_StatusToString(status);
	}
template<typename T, typename... ARGS>
	T operator()(O * o, T (O::*f)(ARGS...), ARGS... args) {
		LPVOID ivf = implementedVirtualFunction(o, f);
		return ((T (*)(O*, ARGS...))info_[ivf].original_)(o, args...);
	}
	~VHook() {
		for (auto i : info_) {
			MH_STATUS status = MH_RemoveHook(i.first);
		}
		//if (status != MH_OK)
			//LOG(WARN) << "Failed to remove minhook " << MH_StatusToString(status);
	}
};


