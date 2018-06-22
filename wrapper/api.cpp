#include "api.h"
#include <algorithm>
#include <fstream>
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>
#include <Windows.h>
#include "util.h"

static const char gamehook_dll_name[] = "gamehook.dll";
static std::string current_dll_name = "";

static std::multimap<int, std::shared_ptr<GameControllerFactory> > factories_;
const std::multimap<int, std::shared_ptr<GameControllerFactory> > & factories() {
	return factories_;
}

size_t registerFactory(GameControllerFactory * h, int priority) {
	for (auto it = factories_.begin(); it != factories_.end(); ) {
		if (it->second.get() == h) it = factories_.erase(it);
		else ++it;
	}
	factories_.insert(std::make_pair(-priority, std::shared_ptr<GameControllerFactory>(h)));
	return factories_.size();
}
size_t unregisterFactory(GameControllerFactory * h) {
	for (auto it = factories_.begin(); it != factories_.end(); ) {
		if (it->second.get() == h) it = factories_.erase(it);
		else ++it;
	}
	return factories_.size();
}

IMPORT size_t dataSize(DataType t) {
	switch (t) {
#define C(T) case DT<T>::type: return sizeof(T)
		DO_ALL_TYPE(C);
#undef C
	}
	return 0;
}

static std::vector<HMODULE> hk_modules;
static std::vector<HMODULE> asi_modules;

static size_t translate_addr(DWORD addr, const IMAGE_NT_HEADERS * nt) {
	const IMAGE_SECTION_HEADER * section_header = reinterpret_cast<const IMAGE_SECTION_HEADER*>(nt + 1);
	for (int i = 0; i < nt->FileHeader.NumberOfSections; i++, section_header++) {
		if (addr >= section_header->VirtualAddress && addr <= section_header->VirtualAddress + section_header->Misc.VirtualSize) {
			return addr - section_header->VirtualAddress + section_header->PointerToRawData;
		}
	}
	LOG(WARN) << "Translate address";
	return 0;
}

HMODULE patchAndLoadLibrary(const char * filename, const char * dll_name) {
	// Create and open a temp file (used for LoadLibrary later)
	TCHAR temp_filename[MAX_PATH];
	TCHAR tmp_path[MAX_PATH];
	DWORD path_len = GetTempPath(MAX_PATH, tmp_path);
	if (path_len == 0 || path_len > MAX_PATH) {
		LOG(WARN) << "Plugin: Failed to get temp path" << filename;
		return 0;
	}
	DWORD file_len = GetTempFileName(tmp_path, TEXT("lib"), 0, temp_filename);

	// Read the DLL
	std::ifstream dll_file(filename, std::ios::binary);
	if (dll_file.fail()) {
		LOG(WARN) << "Plugin: Failed open plugin" << filename;
		return 0;
	}
	std::vector<char> buffer((std::istreambuf_iterator<char>(dll_file)), (std::istreambuf_iterator<char>()));
	dll_file.close();

	// Read the DLL header
	const IMAGE_DOS_HEADER * dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(buffer.data());
	
	if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew == 0x00000000) {
		LOG(WARN) << "Plugin: Wrong dos signature! " << filename;
		return 0;
	}
	const IMAGE_NT_HEADERS * nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(buffer.data() + dos->e_lfanew);
	// Make sure the machine matches
#ifdef _WIN64 
	if(nt->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64) {
		LOG(WARN) << "Plugin '" << filename << "' is 32bit, but 64bit required!";
		return 0;
	}
#else
	if (nt->FileHeader.Machine != IMAGE_FILE_MACHINE_I386) {
		LOG(WARN) << "Plugin '" << filename << "' is 64bit, but 32bit required!";
		return 0;
	}
#endif
	// Get the dll import description
	DWORD addr = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
	size_t offset = translate_addr(addr, nt);
	if (!offset) {
		LOG(WARN) << "Failed to find import description";
		return 0;
	}
	PIMAGE_IMPORT_DESCRIPTOR import_desc = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(buffer.data() + offset);

	// Find and translate the dll name
	for (; import_desc->Name; import_desc++) {
		size_t name_o = translate_addr(import_desc->Name, nt);
		char * name = &buffer[name_o];
		if (strcmp(name, gamehook_dll_name) == 0) {
			const size_t N = strlen(gamehook_dll_name);
			ASSERT(N >= strlen(dll_name));
			memset(name, 0, N);
			strcpy_s(name, N, dll_name);
		}
	}

	// Write the tempfile dll
	std::ofstream tmp_dll_file(temp_filename, std::ios::binary);
	tmp_dll_file.write(buffer.data(), buffer.size());
	tmp_dll_file.close();

	// Load the temp library
	HMODULE r = LoadLibrary(temp_filename);
	if (!r)
		LOG(WARN) << "LoadLibrary failed " << std::hex << GetLastError();
	return r;
}


void loadAPI(const char * dll_name, bool load_hk, bool load_asi) {
	if (dll_name && !current_dll_name.size())
		current_dll_name = dll_name;
	else if (!dll_name && current_dll_name.size())
		dll_name = current_dll_name.c_str();
	LOG(INFO) << "Loading API";
	WIN32_FIND_DATA data;
	// We act as an ASI loader
#ifndef NO_ASI_LOADER
	if (load_asi) {
		HANDLE hFind = FindFirstFile("*.asi", &data);
		if (hFind != INVALID_HANDLE_VALUE) {
			do {
				LOG(INFO) << "Loading asi " << data.cFileName;
				HMODULE dll = LoadLibrary(data.cFileName);
				if (dll)
					asi_modules.push_back(dll);
				else
					LOG(WARN) << "Failed to load asi '" << data.cFileName << "'!";
			} while (FindNextFile(hFind, &data));
			FindClose(hFind);
		}
	}
#endif
	// and hk loader
	if (load_hk) {
		HANDLE hFind = FindFirstFile("*.hk", &data);
		if (hFind != INVALID_HANDLE_VALUE) {
			do {
				LOG(INFO) << "Loading and patching plugin " << data.cFileName;
				HMODULE dll = patchAndLoadLibrary(data.cFileName, dll_name);
				if (dll)
					hk_modules.push_back(dll);
				else
					LOG(WARN) << "Failed to load plugin '" << data.cFileName << "'!";
			} while (FindNextFile(hFind, &data));
			FindClose(hFind);
		}
	}
}
void unloadAPI(bool unload_hk, bool unload_asi) {
	if (unload_hk) {
		for (auto m : hk_modules) {
			CHAR fn[256];
			GetModuleFileName(m, fn, 256);
			LOG(INFO) << "Unloading plugin " << fn;
			FreeLibrary(m);
			DeleteFile(fn);
		}
		hk_modules.clear();
	}
	if (unload_asi) {
		for (auto m : asi_modules) {
			CHAR fn[256];
			GetModuleFileName(m, fn, 256);
			LOG(INFO) << "Unloading plugin " << fn;
			FreeLibrary(m);
			DeleteFile(fn);
		}
		asi_modules.clear();
	}
}
void reloadAPI(bool reload_hk, bool reload_asi) {
	unloadAPI(reload_hk, reload_asi);
	loadAPI(nullptr, reload_hk, reload_asi);
}

