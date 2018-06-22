#include <windows.h>
#include <iostream>
#include "dxgi.h"
#include "dx11.h"
#include "util.h"
#include "api.h"
#include "../minhook/include/MinHook.h"


BOOL WINAPI DllMain(HINSTANCE hInst, DWORD reason, LPVOID) {
	if (reason == DLL_PROCESS_ATTACH) {
		TCHAR DllPath[MAX_PATH], szSystemPath[MAX_PATH];
		GetModuleFileName(hInst, DllPath, MAX_PATH);
		TCHAR *DllName = strrchr(DllPath, '\\');
		LOG(INFO) << "Staring up (fake)" << DllName;

		GetSystemDirectory(szSystemPath, MAX_PATH);
		strcat_s(szSystemPath, DllName);
		initDLL(szSystemPath);
		hookD3D11();
		LOG(INFO) << "Initialized: " << szSystemPath;

		LOG(INFO) << "Loading the API";
		loadAPI(DllName+1);
	}

	if (reason == DLL_PROCESS_DETACH) {
		LOG(INFO) << "Cleaning up";
		unloadAPI();
		unhookD3D11();
		freeDLL();
	}
	return TRUE;
}