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
#include <tchar.h>

#define BUFFER_SIZE 1048576

#define WIN32_STREAM_ID_SIZE	offsetof(WIN32_STREAM_ID, cStreamName)	// not sizeof(WIN32_STREAM_ID) because of the ANY_SIZE array at the end

#define BLOCKING_STREAM_NAME	TEXT(":Zone.Identifier:$DATA")

#define MIN(a, b)				(((a) < (b)) ? (a) : (b))

#ifndef FIND_FIRST_EX_LARGE_FETCH
#define FIND_FIRST_EX_LARGE_FETCH 0
#endif
#ifndef FindExInfoBasic
#define FindExInfoBasic FindExInfoStandard
#endif

static unblock_count = 0;
static failed_count  = 0;
static skipped_count = 0;

//#include "WSI-Inspector.c"

#define UNBLOCK_CRITICAL	-2
#define UNBLOCK_FAILED		-1
#define UNBLOCK_SUCCESS		0
#define UNBLOCK_NO_NEED		1

// Unblocks a file
// The buffer is used internally, and is there so that we only have to allocate once for all files
// Returns one of the UNBLOCK_* values
static int unblock(TCHAR *file, LPBYTE buffer) {
	DWORD err, retval = UNBLOCK_SUCCESS;
	HANDLE f, t;
	TCHAR temp[MAX_PATH];
	LPVOID f_cntx = NULL, t_cntx = NULL;
	WIN32_STREAM_ID *wsi = (WIN32_STREAM_ID*)buffer;
	BOOL skipped_something = FALSE;

	// Get a temporary file
	if (!GetTempFileName(TEXT("."), TEXT("~ub"), 0, temp)) {
		_ftprintf(stderr, TEXT("! Unable to get temporary files: %u\n"), GetLastError());
		return UNBLOCK_CRITICAL;
	}

	// Open the file for reading
	// FILE_WRITE_ATTRIBUTES is required for BackupRead...
	f = CreateFile(file, FILE_GENERIC_READ | FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (f == INVALID_HANDLE_VALUE) {
		_ftprintf(stderr, TEXT("! Unable to open file '%s': %u\n"), file, GetLastError());
		DeleteFile(temp);
		++failed_count;
		return UNBLOCK_FAILED;
	}

	// Open the temporary file
	t = CreateFile(temp, FILE_ALL_ACCESS, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, f);
	if (t == INVALID_HANDLE_VALUE) {
		_ftprintf(stderr, TEXT("! Unable to create temporary files: %u\n"), GetLastError());
		CloseHandle(f);
		DeleteFile(temp);
		return UNBLOCK_CRITICAL;
	}

	while (TRUE) {
		BOOL skip;
		DWORD read = 0, wrote = 0;
		ZeroMemory(buffer, BUFFER_SIZE); // this is required because the stream name stuff doesn't read properly...

		// Read in the stream information
		if (!BackupRead(f, buffer, WIN32_STREAM_ID_SIZE, &read, FALSE, TRUE, &f_cntx) || read != WIN32_STREAM_ID_SIZE) { break; }
		if (wsi->dwStreamNameSize > 0) { // read in the stream name
			if (!BackupRead(f, buffer+WIN32_STREAM_ID_SIZE, wsi->dwStreamNameSize, &read, FALSE, TRUE, &f_cntx) || read != wsi->dwStreamNameSize) { break; }
			skip = _tcscmp(wsi->cStreamName, BLOCKING_STREAM_NAME) == 0; // is it the right stream?
		} else {
			skip = FALSE;
		}

//		dumpWSI(wsi);

		if (skip) {
			// Skipping this stream, seek to it's end
			skipped_something = TRUE;
			if (wsi->Size.QuadPart > 0) {
				DWORD lo, hi;
				if (!BackupSeek(f, wsi->Size.LowPart, wsi->Size.HighPart, &lo, &hi, &f_cntx) || lo != wsi->Size.LowPart || hi != wsi->Size.HighPart) { break; }
			}
		} else {
			// Copying this stream
			LONGLONG sz = wsi->Size.QuadPart;
			// Write the stream information
			if (!BackupWrite(t, buffer, WIN32_STREAM_ID_SIZE+wsi->dwStreamNameSize, &wrote, FALSE, TRUE, &t_cntx) || wrote != WIN32_STREAM_ID_SIZE+wsi->dwStreamNameSize) { break; }
			while (sz) {
				// Loop read and writing chunks of data
				if (!BackupRead(f, buffer, MIN(BUFFER_SIZE, sz), &read, FALSE, TRUE, &f_cntx) || !read) { break; }
				if (!BackupWrite(t, buffer, read, &wrote, FALSE, TRUE, &t_cntx) || wrote != read) { break; }
				sz -= read;
			}
			// The loop didn't go to completion, we failed somewhere
			if (sz) { break; }
		}
	}

	// Save the last error
	err = GetLastError();

	// Finalize the context structures (abort)
	if (f_cntx) BackupRead(f, NULL, 0, NULL, TRUE, FALSE, &f_cntx);
	if (t_cntx) BackupWrite(t, NULL, 0, NULL, TRUE, FALSE, &t_cntx);

	if (err != ERROR_SUCCESS && err != ERROR_HANDLE_EOF) {
		_ftprintf(stderr, TEXT("! Failed to unblock file '%s': %u\n"), file, GetLastError());
		++failed_count;
		retval = UNBLOCK_FAILED;
	} else {
		if (skipped_something) {
			// We actually skipped a stream, copy temp file to the original file
			if (!CopyFile(temp, file, FALSE)) {
				_ftprintf(stderr, TEXT("! Failed to copy temporary file to '%s': %u\n"), file, GetLastError());
				++failed_count;
				retval = UNBLOCK_FAILED;
			} else {
				++unblock_count;
				_tprintf(TEXT("Unblocked '%s'\n"), file);
			}
		} else {
			// We didn't do anything, just ignore
			//_tprintf(TEXT("No block found for '%s'\n"), file);
			++skipped_count;
			retval = UNBLOCK_NO_NEED;
		}
	}

	// Cleanup
	CloseHandle(f);
	CloseHandle(t);

	DeleteFile(temp); // just in case FILE_FLAG_DELETE_ON_CLOSE didn't works

	return retval;
}

// Combines a directory and a file into a full path, stored in dst
TCHAR *makeFullPath(TCHAR *dir, TCHAR *file, TCHAR *dst) {
	DWORD dir_len = _tcslen(dir);
	DWORD file_len = _tcslen(file);
	// Remove wildcards and slashes
	while (dir_len > 0 && dir[dir_len-1] == L'*')	dir[--dir_len] = 0;
	while (dir_len > 0 && dir[dir_len-1] == L'\\')	dir[--dir_len] = 0;
	_tcsncpy(dst, dir, dir_len);
	dst[dir_len++] = L'\\'; // add back just one slash
	_tcsncpy(dst+dir_len, file, file_len);
	dst[dir_len+file_len] = 0;
	return dst;
}

// Recursively unblocks all files in a directory and all subdirectories
static int unblock_dir(TCHAR *dir, LPBYTE buffer) {
	WIN32_FIND_DATA ffd;
	TCHAR full[MAX_PATH+5]; // need some extra room for the manipulations that we do
	HANDLE hFind;
	DWORD dwError;
	DWORD len = _tcslen(dir);
	while (len > 0 && dir[len-1] == L'\\') // remove slashes
		dir[--len] = 0;
	dir[len  ] = TEXT('\\'); // add the wildcard
	dir[len+1] = TEXT('*');
	dir[len+2] = 0;
	hFind = FindFirstFileEx(dir, FindExInfoBasic, &ffd, FindExSearchNameMatch, NULL, FIND_FIRST_EX_LARGE_FETCH);
	if (hFind == INVALID_HANDLE_VALUE) {
		dwError = GetLastError();
		if (dwError != ERROR_FILE_NOT_FOUND && dwError != ERROR_ACCESS_DENIED) {
			_ftprintf(stderr, TEXT("! FindFirstFileEx failed for '%s': %u\n"), dir, dwError);
			++failed_count;
			return UNBLOCK_FAILED;
		}
	} else {
		do {
			if (_tcscmp(ffd.cFileName, TEXT("..")) == 0 || _tcscmp(ffd.cFileName, TEXT(".")) == 0)
				continue;
			makeFullPath(dir, ffd.cFileName, full);
			if (ffd.dwFileAttributes != INVALID_FILE_ATTRIBUTES && (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
				dwError = unblock_dir(full, buffer);
			} else {
				dwError = unblock(full, buffer);
			}
			if (dwError == UNBLOCK_CRITICAL) {
				FindClose(hFind);
				return UNBLOCK_CRITICAL;
			}
		} while (FindNextFile(hFind, &ffd) != 0);
		dwError = GetLastError();
		FindClose(hFind);
		if (dwError != ERROR_NO_MORE_FILES) {
			_ftprintf(stderr, TEXT("! FindNextFile failed: %u\n"), dwError);
			++failed_count;
			return UNBLOCK_FAILED;
		}
	}
	return UNBLOCK_SUCCESS;
}

#include "mingw-unicode.c"
int _tmain(int argc, _TCHAR* argv[]) {
	int i;
	DWORD retval;
	LPBYTE buffer;

	if (argc < 2) {
		_tprintf(TEXT("Usage: %s file1 [file2 ...]\n"), argv[0]);
		return 1;
	}

	buffer = LocalAlloc(0, BUFFER_SIZE);
	for (i = 1; i < argc && retval != UNBLOCK_CRITICAL; ++i) {
		DWORD attrib = GetFileAttributes(argv[i]);
		if (attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY) != 0) {
			TCHAR *dir = _tcscpy(LocalAlloc(0, MAX_PATH+5), argv[i]);
			retval = unblock_dir(dir, buffer);
			LocalFree(dir);
		} else {
			retval = unblock(argv[i], buffer);
		}
	}
	LocalFree(buffer);

	_tprintf(TEXT("Unblocked:%6u files\n"), unblock_count);
	_tprintf(TEXT("Failed:   %6u files\n"), failed_count);
	_tprintf(TEXT("Skipped:  %6u files\n"), skipped_count);

	return retval == UNBLOCK_CRITICAL ? 1 : 0;
}
