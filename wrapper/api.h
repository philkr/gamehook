#pragma once
#ifndef IMPORT
#define IMPORT __declspec(dllexport)
#endif
#include "../SDK/sdk.h"

void loadAPI(const char * dll_name, bool load_hk=true, bool load_asi=true);
void unloadAPI(bool unload_hk=true, bool unload_asi=true);
void reloadAPI(bool reload_hk=true, bool reload_asi=true);
