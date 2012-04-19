#include "FindProcess.h"

#include <tchar.h>

// Macros for function definition and loading
#define FUNC(n, t, r, ...) typedef r (WINAPI* t)(__VA_ARGS__); t n;
#define LOAD_FUNC(n, t, l)		(n = (t)GetProcAddress(l, #n))
#define LOAD_FUNC_A(n, t, l)	(n = (t)GetProcAddress(l, #n "A"))
#define LOAD_FUNC_W(n, t, l)	(n = (t)GetProcAddress(l, #n "W"))
#ifdef _UNICODE
#define LOAD_FUNC_T LOAD_FUNC_W
#else
#define LOAD_FUNC_T LOAD_FUNC_A
#endif

typedef struct _MODULEINFO {
    LPVOID lpBaseOfDll;
    DWORD SizeOfImage;
    LPVOID EntryPoint;
} MODULEINFO, *LPMODULEINFO;

FUNC(EnumProcesses,			ENUMPROCESSES,			BOOL,	DWORD*,DWORD,DWORD*)
FUNC(EnumProcessModules,	ENUMPROCESSMODULES,		BOOL,	HANDLE,HMODULE*,DWORD,LPDWORD)
FUNC(GetModuleBaseName,		GETMODULEBASENAME,		DWORD,	HANDLE,HMODULE,LPTSTR,DWORD)
FUNC(GetModuleInformation,	GETMODULEINFORMATION,	BOOL,	HANDLE,HMODULE,LPMODULEINFO,DWORD)

HMODULE psapiLib = NULL;

BOOL loadPSAPI() {
	if (!psapiLib && !(psapiLib = LoadLibrary(TEXT("PSAPI.dll")))) { return FALSE; }
	if (!LOAD_FUNC(EnumProcesses,		ENUMPROCESSES,			psapiLib)) { return FALSE; }
	if (!LOAD_FUNC(EnumProcessModules,	ENUMPROCESSMODULES,		psapiLib)) { return FALSE; }
	if (!LOAD_FUNC_T(GetModuleBaseName,	GETMODULEBASENAME,		psapiLib)) { return FALSE; }
	if (!LOAD_FUNC(GetModuleInformation,GETMODULEINFORMATION,	psapiLib)) { return FALSE; }
	return TRUE;
}
BOOL freePSAPI() {
	BOOL retval = psapiLib ? FreeLibrary(psapiLib) : TRUE;
	psapiLib = NULL;
	return retval;
}

DWORD FindProcess(LPCTSTR name, BYTE **base) {
	DWORD pids[1024], size, count, i, nameLen = _tcslen(name);
	MODULEINFO modinfo;
	if (!loadPSAPI()) return 0;
	if (!EnumProcesses(pids, sizeof(pids), &size)) {
		freePSAPI();
		return 0;
	}
	count = size / sizeof(DWORD);
	for (i = 0; i < count; i++) {
		if (pids[i] != 0) {
			TCHAR pName[MAX_PATH] = TEXT("");
			HANDLE proc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pids[i]);
			if (proc) {
				HMODULE hMod;
				DWORD cbNeeded;
				if (EnumProcessModules(proc, &hMod, sizeof(hMod), &cbNeeded)) {
					GetModuleBaseName(proc, hMod, pName, sizeof(pName)/sizeof(TCHAR));
					if (CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE, pName, _tcslen(pName), name, nameLen) == CSTR_EQUAL) {
						GetModuleInformation(proc, hMod, &modinfo, sizeof(MODULEINFO));
						CloseHandle(proc);
						freePSAPI();
						*base = modinfo.lpBaseOfDll;
						return pids[i];
					}
				}
				CloseHandle(proc);
			}
		}
	}
	SetLastError(ERROR_NOT_FOUND);
	freePSAPI();
	return 0;
}
