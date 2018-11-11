#include "dxgi.h"
#include <dxgi.h>
#include <InitGuid.h>

static HRESULT NotImpl() {
	return E_NOTIMPL;
}
static const FARPROC not_impl = (FARPROC)*NotImpl;

#define VISIT_DXGI(F) \
F(ApplyCompatResolutionQuirking) \
F(CompatString) \
F(CompatValue) \
F(DXGIDumpJournal) \
F(DXGIRevertToSxS) \
F(PIXBeginCapture) \
F(PIXEndCapture) \
F(PIXGetCaptureState) \
F(SetAppCompatStringPointer) \
F(CreateDXGIFactory1) \
F(CreateDXGIFactory2) \
F(CreateDXGIFactory) \
F(DXGID3D10CreateDevice) \
F(DXGID3D10CreateLayeredDevice) \
F(DXGID3D10ETWRundown) \
F(DXGID3D10GetLayeredDeviceSize) \
F(DXGID3D10RegisterLayers) \
F(DXGIGetDebugInterface1) \
F(DXGIReportAdapterConfiguration) \
F(D3DKMTCloseAdapter) \
F(D3DKMTDestroyAllocation) \
F(D3DKMTDestroyContext) \
F(D3DKMTDestroyDevice) \
F(D3DKMTDestroySynchronizationObject) \
F(D3DKMTQueryAdapterInfo) \
F(D3DKMTSetDisplayPrivateDriverFormat) \
F(D3DKMTSignalSynchronizationObject) \
F(D3DKMTUnlock) \
F(OpenAdapter10) \
F(OpenAdapter10_2) \
F(D3DKMTCreateAllocation) \
F(D3DKMTCreateContext) \
F(D3DKMTCreateDevice) \
F(D3DKMTCreateSynchronizationObject) \
F(D3DKMTEscape) \
F(D3DKMTGetContextSchedulingPriority) \
F(D3DKMTGetDeviceState) \
F(D3DKMTGetDisplayModeList) \
F(D3DKMTGetMultisampleMethodList) \
F(D3DKMTGetRuntimeData) \
F(D3DKMTGetSharedPrimaryHandle) \
F(D3DKMTLock) \
F(D3DKMTOpenAdapterFromHdc) \
F(D3DKMTOpenResource) \
F(D3DKMTPresent) \
F(D3DKMTQueryAllocationResidency) \
F(D3DKMTQueryResourceInfo) \
F(D3DKMTRender) \
F(D3DKMTSetAllocationPriority) \
F(D3DKMTSetContextSchedulingPriority) \
F(D3DKMTSetDisplayMode) \
F(D3DKMTSetGammaRamp) \
F(D3DKMTSetVidPnSourceOwner) \
F(D3DKMTWaitForSynchronizationObject) \
F(D3DKMTWaitForVerticalBlankEvent)

struct DXGI {
	HMODULE dll = nullptr;
#define DCL(x) FARPROC x = not_impl;
	VISIT_DXGI(DCL);
#undef DCL
	static FARPROC Get(HMODULE dll, const char * n) {
		FARPROC r = GetProcAddress(dll, n);
		if (!n) return not_impl;
		return r;
	}
	void load(const char *path) {
		dll = LoadLibrary(path);
		// Load dll functions
#define LD(x) x = Get(dll, #x);
		VISIT_DXGI(LD);
#undef LD
	}
	void unload() {
		FreeLibrary(dll);
		dll = 0;
	}
};
static DXGI dxgi;
void initDLL(const char *path) {
	dxgi.load(path);
}
void freeDLL() {
	dxgi.unload();
}
#ifdef _WIN64
// Not sure if this works in debug mode, even in release it's very very UGLY
#define DF(x) INT_PTR Fake ## x() { return dxgi.x(); }
#else
#define DF(x) __declspec(naked) void Fake ## x() { _asm { jmp[dxgi.x] } }
#endif
VISIT_DXGI(DF);
#undef DF
