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

#define BUFFER_SIZE 1048576

// Enables a privilege by name (SE_XXX_NAME)
static BOOL EnablePriv(LPCTSTR lpName) {
	HANDLE hToken;
	LUID luid;
	TOKEN_PRIVILEGES tkprivs;
	BOOL retval = FALSE;
	ZeroMemory(&tkprivs, sizeof(tkprivs));
	if (!OpenProcessToken(GetCurrentProcess(), (TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY), &hToken)) { return FALSE; }
	if (LookupPrivilegeValue(NULL, lpName, &luid)){
		tkprivs.PrivilegeCount = 1;
		tkprivs.Privileges[0].Luid = luid;
		tkprivs.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		retval = AdjustTokenPrivileges(hToken, FALSE, &tkprivs, sizeof(tkprivs), NULL, NULL);
	}
	CloseHandle(hToken);
	return retval;
}

static LogError(TCHAR *text) {
	_ftprintf(stderr, TEXT("! %s: %u\n"), text, GetLastError());
}

// Adds the readonly flag to a file
static AddReadOnly(TCHAR *file) {
	DWORD attrib = GetFileAttributes(file);
	if (attrib != INVALID_FILE_ATTRIBUTES) {
		SetFileAttributes(file, attrib | FILE_ATTRIBUTE_READONLY);
	}
}

// Checks the attributes of the destination file
// FALSE is returned if the destination file cannot be made
// TRUE otherwise
static BOOL CheckAttributes(TCHAR *src, TCHAR **pdest, BOOL *readonly) {
	TCHAR *dest = *pdest;
	DWORD attrib = GetFileAttributes(dest);
	if (attrib == INVALID_FILE_ATTRIBUTES) {
		if (GetLastError() != ERROR_FILE_NOT_FOUND) { // File not found is good for the destination
			LogError(TEXT("Unable to retrieve attributes of destination file"));
			_ftprintf(stderr, TEXT("We will try to copy anyways, but it may not work.\n"));
		}
	} else {
		if (attrib & FILE_ATTRIBUTE_DIRECTORY) {
			// If destination is a directory, we will try to descend into it
			if (src==NULL) {
				// Will into a subdirectory
				_ftprintf(stderr, TEXT("The destination directory contains a directory with the given filename\n"));
				LocalFree(dest);
				return FALSE;
			}
			// Get a new destination file name by appending the source filename onto the destination directory
			TCHAR *filename = _tcsrchr(src, TEXT('\\'));
			int len = _tcslen(dest);
			*pdest = dest = _tcscpy(LocalAlloc(0, len+_tcslen(filename)+2), dest); // copy dest to a new, longer, string
			if (dest[len-1] != TEXT('\\')) {
				dest[len] = TEXT('\\');
			}
			_tcscat(dest, filename);
			// Try to check attributes again, src==NULL signifies that we already went into a directory
			return CheckAttributes(NULL, pdest, readonly);
		}
		// Get the read-only flag
		*readonly = attrib & FILE_ATTRIBUTE_READONLY;
		if (*readonly) {
			SetFileAttributes(dest, attrib & !FILE_ATTRIBUTE_READONLY);
		}
	}
	return TRUE;
}

int _tmain(int argc, _TCHAR* argv[]) {
	TCHAR *src, *dest;
	BOOL backup, readonly, writeFailed = FALSE;
	HANDLE s, d;
	DWORD attrib, read = 0, write = 0, retval = 0;
	LARGE_INTEGER size, cur, zero = {0};
	LPVOID buffer, s_cntx = NULL, d_cntx = NULL;

	if (argc != 3) {
		_tprintf(TEXT("Usage: %s source destination\n"), argv[0]);
		_tprintf(TEXT("Destination can a file or directory\n"));
		return 1;
	}

	src = argv[1];
	dest = argv[2];

	backup = EnablePriv(SE_BACKUP_NAME) && EnablePriv(SE_RESTORE_NAME);
	if (!backup) {
		LogError(TEXT("Unable to obtain backup privileges"));
		_ftprintf(stderr, TEXT("Make sure that you are an administrator or running this elevated.\n"));
		_ftprintf(stderr, TEXT("We will try to copy anyways, but it may not work.\n"));
	}

	attrib = GetFileAttributes(src);
	if (attrib == INVALID_FILE_ATTRIBUTES) {
		LogError(TEXT("Unable to retrieve attributes for the source file"));
		_ftprintf(stderr, TEXT("We will try to copy anyways, but it may not work.\n"));
	} else if (attrib & FILE_ATTRIBUTE_DIRECTORY) {
		_ftprintf(stderr, TEXT("Source must be a file\n"));
		return 1;
	}

	if (!CheckAttributes(src, &dest, &readonly)) {
		return 1;
	}

	//It wants FILE_WRITE_ATTRIBUTES to finish up the job, but I say no!
	s = CreateFile(src, FILE_READ_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (s == INVALID_HANDLE_VALUE) {
		LogError(TEXT("Unable to open source file"));
		if (readonly) { AddReadOnly(dest); }
		return 1;
	}

	d = CreateFile(dest, FILE_WRITE_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, CREATE_ALWAYS, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if (d == INVALID_HANDLE_VALUE) {
		LogError(TEXT("Unable to create the destination file"));
		CloseHandle(s);
		if (readonly) { AddReadOnly(dest); }
		return 1;
	}

	if (!GetFileSizeEx(s, &size)) {
		LogError(TEXT("Failed to get size of source file"));
		retval = 1;
	} else if (!SetFilePointerEx(d, size, NULL, FILE_BEGIN) || !SetEndOfFile(d) || !SetFilePointerEx(d, zero, NULL, FILE_BEGIN)) {
		LogError(TEXT("Failed to set the size of destination file"));
		retval = 1;
	} else if (!(buffer = LocalAlloc(0, BUFFER_SIZE))) {
		LogError(TEXT("Failed to allocate the buffer"));
		retval = 1;
	}
	if (retval) {
		CloseHandle(s);
		CloseHandle(d);
		if (readonly) { AddReadOnly(dest); }
		return 1;
	}

	SetLastError(ERROR_SUCCESS); // CreateFile can succeed and set the last error to ERROR_ALREADY_EXISTS which looks like an error later

	while (BackupRead(s, buffer, BUFFER_SIZE, &read, FALSE, FALSE, &s_cntx) && read) {
		if (!BackupWrite(d, buffer, read, &write, FALSE, FALSE, &d_cntx)) {
			LogError(TEXT("BackupWrite failed"));
			writeFailed = TRUE;
			break;
		}
		if (write != read) {
			_ftprintf(stderr, TEXT("! Some data didn't write but this program doesn't handle that situation...\n"));
		}
	}
	
	retval = GetLastError();
	if (retval == ERROR_ACCESS_DENIED && !writeFailed && SetFilePointerEx(d, zero, &cur, FILE_CURRENT) && cur.QuadPart == size.QuadPart) {
		// The whole file was copied
		// BackupRead() sets ERROR_ACCESS_DENIED without FILE_WRITE_ATTRIBUTES though
		retval = ERROR_HANDLE_EOF;
	}

	if (s_cntx) BackupRead(s, NULL, 0, NULL, TRUE, FALSE, &s_cntx);
	if (d_cntx) BackupWrite(d, NULL, 0, NULL, TRUE, FALSE, &d_cntx);

	CloseHandle(s);
	CloseHandle(d);

	LocalFree(buffer);

	SetLastError(retval);

	if (retval != ERROR_SUCCESS && retval != ERROR_HANDLE_EOF) {
		LogError(TEXT("Failed to transfer data"));
		retval = 1;
	} else {
		if (readonly) { AddReadOnly(dest); }
		_tprintf(TEXT("Copied %s to %s\n"), src, dest);
	}

	if (dest != argv[2]) LocalFree(dest);

	return retval;
}
