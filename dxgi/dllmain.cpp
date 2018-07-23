#include <windows.h>
#include <iostream>
#include "blacklist.h"
#include "dxgi.h"
#include "dx11.h"
#include "util.h"
#include "api.h"

static bool blacklisted = false;

BOOL WINAPI DllMain(HINSTANCE hInst, DWORD reason, LPVOID) {
	if (reason == DLL_PROCESS_ATTACH) {
		TCHAR DllPath[MAX_PATH], szSystemPath[MAX_PATH];
		// Find and load the real dxgi.dll
		GetModuleFileName(hInst, DllPath, MAX_PATH);
		TCHAR *DllName = strrchr(DllPath, '\\');

		GetSystemDirectory(szSystemPath, MAX_PATH);
		strcat_s(szSystemPath, DllName);
		initDLL(szSystemPath);

		blacklisted = isBlackListed();
		if (!blacklisted) {
			LOG(INFO) << "Staring up (fake)" << DllName;
			LOG(INFO) << "Initialized: " << szSystemPath;
			hookD3D11();

			LOG(INFO) << "Loading the API";
			loadAPI(DllName + 1);
		}
	}

	if (reason == DLL_PROCESS_DETACH) {
		if (!blacklisted) {
			LOG(INFO) << "Cleaning up";
			unloadAPI();
			unhookD3D11();
		}
		freeDLL();
	}
	return TRUE;
}
