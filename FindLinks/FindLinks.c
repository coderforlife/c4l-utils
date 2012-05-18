#ifndef _WIN32_WINNT
// Usually I would try to support XP (0x0501) but it does not support FindFirstFileNameW or FindNextFileNameW
#define _WIN32_WINNT 0x0600
#endif

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>

#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>

/* No longer needed
#ifdef __GNUC__
#define DYN_LOAD_FUNCS
#endif
*/

#ifdef DYN_LOAD_FUNCS
// Macros for function definition and loading
#define FUNC(n, r, ...) typedef r (WINAPI* FUNC_##n)(__VA_ARGS__); FUNC_##n n;
#define LOAD_FUNC(n, l)		(n = (FUNC_##n)GetProcAddress(l, #n))
#define LOAD_FUNC_A(n, l)	(n = (FUNC_##n)GetProcAddress(l, #n "A"))
#define LOAD_FUNC_W(n, l)	(n = (FUNC_##n)GetProcAddress(l, #n "W"))
#define LOAD_FUNC_T UNI_WA(LOAD_FUNC_)

FUNC(FindFirstFileNameW, HANDLE, LPCWSTR lpFileName, DWORD dwFlags,  LPDWORD StringLength, PWCHAR LinkName);
FUNC(FindNextFileNameW, BOOL, HANDLE hFindStream, LPDWORD StringLength, PWCHAR LinkName);
#endif

static unsigned __int64 toUI64(DWORD high, DWORD low) {
	ULARGE_INTEGER x = {low, high};
	return x.QuadPart;
}

int _tmain(int argc, _TCHAR* argv[]) {
	HANDLE h;
	BY_HANDLE_FILE_INFORMATION info;
	TCHAR *filename, linkname[MAX_PATH];
	DWORD length;
	if (argc != 2) {
		_tprintf(TEXT("Usage: %s file\n"), argv[0]);
		return 1;
	}
	filename = argv[1];
	h = CreateFile(filename, FILE_READ_ATTRIBUTES, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (h == INVALID_HANDLE_VALUE) {
		_ftprintf(stderr, TEXT("Failed to open '%s': %u\n"), filename, GetLastError());
	} else {
		if (!GetFileInformationByHandle(h, &info)) {
			_ftprintf(stderr, TEXT("Failed to get the file info for '%s': %u\n"), filename, GetLastError());
		} else {
			_tprintf(TEXT("File Index:      0x%016I64X\n"), toUI64(info.nFileIndexHigh, info.nFileIndexLow));
			_tprintf(TEXT("Volume Serial #: 0x%08X\n"), info.dwVolumeSerialNumber);
			_tprintf(TEXT("Number of Links: %u\n"), info.nNumberOfLinks);
		}
		CloseHandle(h);
	}
#ifdef DYN_LOAD_FUNCS
	if (!LOAD_FUNC(FindFirstFileNameW, GetModuleHandle(TEXT("Kernel32.dll"))) || !LOAD_FUNC(FindNextFileNameW, GetModuleHandle(TEXT("Kernel32.dll")))) {
		_tprintf(TEXT("Enumerating links requires Windows Vista or later\n"), argv[0]);
		return 0;
	}
#endif
	length = MAX_PATH;
	h = FindFirstFileNameW(filename, 0, &length, linkname);
	if (h == INVALID_HANDLE_VALUE) {
		_ftprintf(stderr, TEXT("Unable to enumerate links for '%s': %u\n"), filename, GetLastError());
	} else {
		_tprintf(TEXT("Links:\n  %s\n"), linkname);
		length = MAX_PATH;
		while (FindNextFileNameW(h, &length, linkname)) {
			_tprintf(TEXT("  %s\n"), linkname);
			length = MAX_PATH;
		}
		if (GetLastError() != ERROR_HANDLE_EOF) {
			_ftprintf(stderr, TEXT("Failed to finish enumerating links: %u\n"), filename, GetLastError());
		}
		CloseHandle(h);
	}
	return 0;
}
