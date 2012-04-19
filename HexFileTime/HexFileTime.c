#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_SWPRINTFS

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

// Converts a FILETIME into a 64-bit integer
// Note: using *(unsigned __int64*)&ft is REALLY bad. It causes alignment problems on 64-bit machines.
static unsigned __int64 ft2i(FILETIME ft) {
	ULARGE_INTEGER i = { ft.dwLowDateTime, ft.dwHighDateTime };
	return i.QuadPart;
}

// Convert a FILETIME to a local SYSTEMTIME so that it can be printed
static BOOL FileTimeToLocalTime(FILETIME ft, SYSTEMTIME *lt) {
	SYSTEMTIME utc;
	return FileTimeToSystemTime(&ft, &utc) && SystemTimeToTzSpecificLocalTime(NULL, &utc, lt);
}


// Shorthand to get all the parts of a SYSTEMTIME
#define DATE_PARTS(x) x.wMonth, x.wDay, x.wYear, x.wHour, x.wMinute, x.wSecond

// Print out the creation, modification, and accessed times for a file.
static void printTimes(HANDLE h) {
	FILETIME create, modify, access;
	SYSTEMTIME c = {0}, m = {0}, a = {0};
	if (!GetFileTime(h, &create, &access, &modify)) { _tprintf(TEXT("! Error getting file times: %u\n"), GetLastError()); return; }

	//handle errors:
	if (!FileTimeToLocalTime(create, &c)) { _tprintf(TEXT("! Error getting local creation time: %u\n"), GetLastError()); }
	if (!FileTimeToLocalTime(modify, &m)) { _tprintf(TEXT("! Error getting local modification time: %u\n"), GetLastError()); }
	if (!FileTimeToLocalTime(access, &a)) { _tprintf(TEXT("! Error getting local accessed time: %u\n"), GetLastError()); }

	_tprintf(TEXT("Creation Time:     %016I64X  %2d/%02d/%4d %2d:%02d:%02d\n"), ft2i(create), DATE_PARTS(c));
	_tprintf(TEXT("Modification Time: %016I64X  %2d/%02d/%4d %2d:%02d:%02d\n"), ft2i(modify), DATE_PARTS(m));
	_tprintf(TEXT("Last Access Time:  %016I64X  %2d/%02d/%4d %2d:%02d:%02d\n"), ft2i(access), DATE_PARTS(a));
}

// Convert a single TCHAR (0-9, A-F) into a nibble (0-15). Must be a valid uppercase letter / number.
#define HEX2NIB(x) ((x)-((_istdigit(x)) ? TEXT('0') : (TEXT('A')-10)))

// Converts a hex string to a FILETIME.
// The hex string may start with 0x or 0X and be upper or lowercase.
// NULL is returned on error (invalid chars, too long, too short, or too big)
// Otherwise ft is returned
static FILETIME *getTime(TCHAR *s, FILETIME *ft) {
	size_t len = _tcslen(s), i;
	ft->dwHighDateTime = ft->dwLowDateTime = 0;
	_tcsupr(s); // all uppercase
	// check if it starts with '0X' for hex
	if (len > 2 && s[0] == TEXT('0') && s[1] == TEXT('X')) {
		s += 2; // skip the '0X'
		len -= 2;
	}
	if (len == 0 || len > 16) { return NULL; } // 16 hex digits max in a 64-bit number
	// Go through each digit and check if it's a hex digit and add it to the time
	for (i = 0; i < len; ++i) {
		if (!_istxdigit(s[i])) { // invalid string
			return NULL;
		} else {
			int x = HEX2NIB(s[i]), p = len-i-1;
			if (p < 8) {
				ft->dwLowDateTime  |= (x << 4*p);
			} else {
				ft->dwHighDateTime |= (x << 4*(p-8));
			}
		}
	}
	// FILETIME cannot be larger than 0x8000000000000000
	return (ft->dwHighDateTime > 0x80000000u) ? NULL : ft;
}

#include "mingw-unicode.c"
int _tmain(int argc, TCHAR *argv[]) {
	FILETIME preserve = {0xFFFFFFFF, 0xFFFFFFFF}; // tells SetFileTime to not update last accessed time on file open
	HANDLE h;

	if (argc != 2 && argc != 5) {
		_tprintf(TEXT("To get the times of a file: %s file\n"), argv[0]);
		_tprintf(TEXT("To set the times of a file: %s file create modify access\n"), argv[0]);
		_tprintf(TEXT("Create, modify, and access are specified in hex\n"));
		_tprintf(TEXT("You must include all three, however any that you don't want to change put X\n"));
		return 1;
	}

	// Open the file to read and write attributes
	h = CreateFile(argv[1], FILE_WRITE_ATTRIBUTES | FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);
	if (h == INVALID_HANDLE_VALUE) {
		_tprintf(TEXT("! Error opening file: %u\n"), GetLastError());
		return 1;
	}
	SetFileTime(h, NULL, &preserve, NULL);

	_tprintf(TEXT("Current file times:\n"));
	printTimes(h);

	if (argc == 5) {
		FILETIME create, access, modify;
		if (!SetFileTime(h, getTime(argv[2], &create), getTime(argv[3], &access), getTime(argv[4], &modify))) {
			_tprintf(TEXT("! Error setting file times: %u\n"), GetLastError());
		}
		_tprintf(TEXT("\nNew file times:\n"));
		printTimes(h);
	}

	CloseHandle(h);
}
