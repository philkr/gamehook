#include <windows.h>
#include <fstream>
#include <unordered_set>
#include "util.h"

/* This module allows gamehook to be blacklisted for certain apps (e.g. a game launcher).
   You have two options to blacklist gamehook for a certain executable:
     1. You can create a file 'gamehook.blacklist' in the game directory and add all executables to blacklist (one per line)
	 2. You can add the executable name to the blacklist variable below.
*/


static std::unordered_set<std::string> blacklist = {"GTAVLauncher.exe"};

bool isBlackListed() {
	TCHAR exepath[MAX_PATH], exename[MAX_PATH], filename[_MAX_FNAME], ext[_MAX_EXT];
	// Get the executable name
	GetModuleFileName(NULL, exepath, MAX_PATH);
	_splitpath_s(exepath, NULL, 0, NULL, 0, filename, _MAX_FNAME, ext, _MAX_EXT);
	_makepath_s(exename, NULL, NULL, filename, ext);
	std::string s_exename = exename;

	if (blacklist.count(s_exename)) return true;

	std::ifstream fin("gamehook.blacklist");
	if (fin.is_open()) {
		std::string line;
		while (std::getline(fin, line)) {
			if (line == s_exename)
				return true;
		}
	}
	return false;
}
