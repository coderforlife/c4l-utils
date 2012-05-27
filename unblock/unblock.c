// unblock: removes the unsafe security block from a file
// Copyright (C) 2010-2012  Jeffrey Bush <jeff@coderforlife.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/gpl.html>.

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_SWPRINTFS

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define WIN32_STREAM_ID_SIZE	offsetof(WIN32_STREAM_ID, cStreamName)	// not sizeof(WIN32_STREAM_ID) because of the ANY_SIZE array at the end

#define BLOCKING_STREAM_NAME	L":Zone.Identifier:$DATA"

#define MIN(a, b)				(((a) < (b)) ? (a) : (b))

#ifndef FIND_FIRST_EX_LARGE_FETCH
#define FIND_FIRST_EX_LARGE_FETCH 0
#endif
#ifndef FindExInfoBasic
#define FindExInfoBasic FindExInfoStandard
#endif

static int unblock_count = 0;
static int failed_count  = 0;
static int skipped_count = 0;

//#include "WSI-Inspector.c"

#define UNBLOCK_FAILED		-1
#define UNBLOCK_SUCCESS		0
#define UNBLOCK_NO_NEED		1

static int _unblock(LPWSTR file) {
	static const FILETIME preserve = {0xFFFFFFFF, 0xFFFFFFFF}; // tells SetFileTime to not update last accessed time on file open

	int retval = UNBLOCK_NO_NEED;
	WCHAR name[1024];
	HANDLE f;
	LPVOID cntx = NULL;
	DWORD err;

	// Open the file for reading
	// FILE_WRITE_ATTRIBUTES is required for BackupRead...
	f = CreateFile(file, FILE_GENERIC_READ | FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	SetFileTime(f, NULL, &preserve, NULL); // do not update last accessed time
	if (f == INVALID_HANDLE_VALUE) {
		fwprintf(stderr, L"! Unable to open file '%s': %u\n", file, GetLastError());
		return UNBLOCK_FAILED;
	}

	for (;;) {
		DWORD read = 0, lo, hi;
		WIN32_STREAM_ID wsi;

		// Read in the stream information
		if (!BackupRead(f, (LPBYTE)&wsi, WIN32_STREAM_ID_SIZE, &read, FALSE, TRUE, &cntx) || read != WIN32_STREAM_ID_SIZE) { break; }
		if (wsi.dwStreamNameSize > 0) { // read in the stream name
			if (wsi.dwStreamNameSize > sizeof(name) - sizeof(WCHAR)) { SetLastError(ERROR_INSUFFICIENT_BUFFER); break; }
			if (!BackupRead(f, (LPBYTE)name, wsi.dwStreamNameSize, &read, FALSE, TRUE, &cntx) || read != wsi.dwStreamNameSize) { break; }
			name[wsi.dwStreamNameSize / sizeof(WCHAR)] = 0;
			if (wcscmp(name, BLOCKING_STREAM_NAME) == 0) {
				// Found blocking stream
				retval = UNBLOCK_SUCCESS;
				break;
			}
		}
		// Skip the stream's data
		if (wsi.Size.QuadPart > 0 && !BackupSeek(f, wsi.Size.LowPart, wsi.Size.HighPart, &lo, &hi, &cntx) || lo != wsi.Size.LowPart || hi != wsi.Size.HighPart) { break; }
	}

	// Check the last error
	err = GetLastError();
	if (retval != UNBLOCK_SUCCESS && err != ERROR_SUCCESS && err != ERROR_HANDLE_EOF)
	{
		fwprintf(stderr, L"! Failed to unblock read file '%s': %u\n", file, err);
		retval = UNBLOCK_FAILED;
	}

	// Cleanup
	if (cntx) BackupRead(f, NULL, 0, NULL, TRUE, FALSE, &cntx);
	CloseHandle(f);

	// Delete the blocking stream and return the status
	return (retval == UNBLOCK_SUCCESS && !DeleteFile(wcscat(file, name))) ? UNBLOCK_FAILED : retval;
}

static int unblock(LPWSTR file) {
	int retval = _unblock(file);
	switch (retval) {
	case UNBLOCK_SUCCESS:
		*wcschr(file+2, ':') = 0;
		wprintf(L"Unblocked '%s'\n", file);
		++unblock_count;
		break;
	case UNBLOCK_NO_NEED: ++skipped_count; break;
	case UNBLOCK_FAILED:  ++failed_count;  break;
	}
	return retval;
}

// Combines a directory and a file into a full path, stored in dst
static WCHAR *makeFullPath(WCHAR *dir, WCHAR *file, WCHAR *dst) {
	DWORD dir_len = wcslen(dir);
	DWORD file_len = wcslen(file);
	// Remove wildcards and slashes
	while (dir_len > 0 && dir[dir_len-1] == L'*')	dir[--dir_len] = 0;
	while (dir_len > 0 && dir[dir_len-1] == L'\\')	dir[--dir_len] = 0;
	wcsncpy(dst, dir, dir_len);
	dst[dir_len++] = L'\\'; // add back just one slash
	wcsncpy(dst+dir_len, file, file_len);
	dst[dir_len+file_len] = 0;
	return dst;
}

// Recursively unblocks all files in a directory and all subdirectories
static int unblock_dir(WCHAR *dir) {
	WIN32_FIND_DATA ffd;
	WCHAR full[MAX_PATH+5]; // need some extra room for the manipulations that we do
	HANDLE hFind;
	DWORD dwError;
	DWORD len = wcslen(dir);
	while (len > 0 && dir[len-1] == L'\\') // remove slashes
		dir[--len] = 0;
	dir[len  ] = L'\\'; // add the wildcard
	dir[len+1] = L'*';
	dir[len+2] = 0;
	hFind = FindFirstFileEx(dir, FindExInfoBasic, &ffd, FindExSearchNameMatch, NULL, FIND_FIRST_EX_LARGE_FETCH);
	if (hFind == INVALID_HANDLE_VALUE) {
		dwError = GetLastError();
		if (dwError != ERROR_FILE_NOT_FOUND && dwError != ERROR_ACCESS_DENIED) {
			fwprintf(stderr, L"! FindFirstFileEx failed for '%s': %u\n", dir, dwError);
			++failed_count;
			return UNBLOCK_FAILED;
		}
	} else {
		do {
			if (wcscmp(ffd.cFileName, L"..") == 0 || wcscmp(ffd.cFileName, L".") == 0)
				continue;
			makeFullPath(dir, ffd.cFileName, full);
			if (ffd.dwFileAttributes != INVALID_FILE_ATTRIBUTES && (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
				dwError = unblock_dir(full);
			} else {
				unblock(full);
			}
		} while (FindNextFile(hFind, &ffd) != 0);
		dwError = GetLastError();
		FindClose(hFind);
		if (dwError != ERROR_NO_MORE_FILES) {
			fwprintf(stderr, L"! FindNextFile failed: %u\n", dwError);
			++failed_count;
			return UNBLOCK_FAILED;
		}
	}
	return UNBLOCK_SUCCESS;
}

int wmain(int argc, wchar_t* argv[]) {
	int i;
	DWORD retval;

	wprintf("unblock Copyright (C) 2010-2012  Jeffrey Bush <jeff@coderforlife.com>\n");
	wprintf("This program comes with ABSOLUTELY NO WARRANTY;\n");
	wprintf("This is free software, and you are welcome to redistribute it\n");
	wprintf("under certain conditions;\n");
	wprintf("See http://www.gnu.org/licenses/gpl.html for more details.\n");
	wprintf("\n");


	if (argc < 2) {
		wprintf(L"Usage: %s file1 [file2 ...]\n", argv[0]);
		return 1;
	}

	for (i = 1; i < argc; ++i) {
		DWORD attrib = GetFileAttributes(argv[i]);
		if (attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY) != 0) {
			WCHAR *dir = wcscpy((WCHAR*)LocalAlloc(0, MAX_PATH+5), argv[i]);
			retval = unblock_dir(dir);
			LocalFree(dir);
		} else {
			unblock(argv[i]);
		}
	}

	wprintf(L"Unblocked:%6u files\n", unblock_count);
	wprintf(L"Failed:   %6u files\n", failed_count);
	wprintf(L"Skipped:  %6u files\n", skipped_count);

	return 0;
}
