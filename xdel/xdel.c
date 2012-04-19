#ifndef _WIN32_WINNT            // Specifies that the minimum required platform is Windows XP.
#define _WIN32_WINNT 0x0501     // Change this to the appropriate value to target other versions of Windows.
#endif

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_SWPRINTFS

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#include <Aclapi.h>
//#include <Sddl.h> // for SID->String conversion (debugging)

#include <shellapi.h>
#pragma comment(lib, "shell32.lib")

#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

#include "util.h"
#include "vector.h"
#include "set.h"

#define BIG_PATH 32767

// Only available on Win 7 / Win Server 2008 R2
// We define them here to acceptable alternatives
#ifndef FIND_FIRST_EX_LARGE_FETCH
#define FIND_FIRST_EX_LARGE_FETCH 0
#endif
#ifndef FindExInfoBasic
#define FindExInfoBasic FindExInfoStandard
#endif

#ifndef FOF_NO_UI
#define FOF_NO_UI FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_NOCONFIRMMKDIR
#endif

static vector *files;

static BOOL LogError(const TCHAR *s, DWORD err) { _ftprintf(stderr, TEXT("! %s Failed: %u\n"), s, err); return FALSE; }
static BOOL LogLastError(const TCHAR *s) { return LogError(s, GetLastError()); }
static BOOL LogFileError(const TCHAR *s, const TCHAR *f, DWORD err) { _ftprintf(stderr, TEXT("! %s '%s': %u\n"), s, f, err); return FALSE; }

// Enable the SeTakeOwnership privilege so that we can take over the key
// Returns TRUE on success, FALSE otherwise
static BOOL EnableTakeOwnershipPriv() {
	HANDLE hToken;
	LUID luid;
	TOKEN_PRIVILEGES tkprivs;
	BOOL retval = FALSE;
	ZeroMemory(&tkprivs, sizeof(tkprivs));
	if (!OpenProcessToken(GetCurrentProcess(), (TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY), &hToken)) {
		return LogLastError(TEXT("OpenProcessToken"));
	}
	if (LookupPrivilegeValue(NULL, SE_TAKE_OWNERSHIP_NAME, &luid)){
		tkprivs.PrivilegeCount = 1;
		tkprivs.Privileges[0].Luid = luid;
		tkprivs.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		if (!(retval = AdjustTokenPrivileges(hToken, FALSE, &tkprivs, sizeof(tkprivs), NULL, NULL))) {
			LogLastError(TEXT("AdjustTokenPrivileges"));
		}
	} else {
		LogLastError(TEXT("LookupPrivilegeValue"));
	}
	CloseHandle(hToken);
	return retval;
}

// Get the SID of the user running this program (not necessarily the logged on user)
// Returns NULL on error or the PSID on success
static PSID GetCurrentSID() {
	HANDLE token;
	PSID sid = NULL;
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
		DWORD size = 0;
		// Get size of the TOKEN_USER data
		GetTokenInformation(token, TokenUser, NULL, 0, &size);
		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
			LogLastError(TEXT("GetTokenInformation"));
		} else {
			TOKEN_USER *user = (TOKEN_USER*)LocalAlloc(0, size);
			if (GetTokenInformation(token, TokenUser, user, size, &size)) {
				// We need to copy the SID because the TOKEN_USER needs to be freed
				size -= sizeof(DWORD);
				sid = (PSID)memcpy(LocalAlloc(0, size), user->User.Sid, size);
			} else {
				LogLastError(TEXT("GetTokenInformation"));
			}
			LocalFree(user);
		}
		CloseHandle(token);
	} else {
		LogLastError(TEXT("OpenProcessToken"));
	}
	return sid;
}

// 'Corrects' the security on a file by taking ownership of it and giving the current user full control
// For directories these will do a complete recursive correction.
static void CorrectSecurity(TCHAR *f, DWORD attrib, BOOL takeownership, PSID sid, PACL acl) {
	if (attrib != INVALID_FILE_ATTRIBUTES) {
		DWORD err;
		if (sid && takeownership) {
			err = SetNamedSecurityInfo(f, SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION, sid, NULL, NULL, NULL);
			if (err != ERROR_SUCCESS) { LogFileError(TEXT("SetNamedSecurityInfo (change owner)"), f, err); }
		}
		if (sid && acl) {
			err = SetNamedSecurityInfo(f, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, acl, NULL);
			if (err != ERROR_SUCCESS) { LogFileError(TEXT("SetNamedSecurityInfo (change DACL)"), f, err); }
		}
		if (attrib & FILE_ATTRIBUTE_DIRECTORY) {
			// Recursively go through the directories
			WIN32_FIND_DATA ffd;
			TCHAR full[BIG_PATH+5], *file = copyStr(f);
			HANDLE hFind;
			DWORD dwError;
			DWORD len = _tcslen(file);
			while (len > 0 && file[len-1] == L'\\')
				file[--len] = 0;
			file[len  ] = TEXT('\\');
			file[len+1] = TEXT('*');
			file[len+2] = 0;
			hFind = FindFirstFileEx(file, FindExInfoBasic, &ffd, FindExSearchNameMatch, NULL, FIND_FIRST_EX_LARGE_FETCH);
			if (hFind == INVALID_HANDLE_VALUE) {
				dwError = GetLastError();
				if (dwError != ERROR_FILE_NOT_FOUND && dwError != ERROR_ACCESS_DENIED)
					LogFileError(TEXT("FindFirstFileEx failed for"), file, dwError);
			} else {
 				do {
					if (_tcscmp(ffd.cFileName, TEXT("..")) == 0 || _tcscmp(ffd.cFileName, TEXT(".")) == 0)
						continue;
					CorrectSecurity(makeFullPath(f, ffd.cFileName, full), ffd.dwFileAttributes, takeownership, sid, acl);
				} while (FindNextFile(hFind, &ffd) != 0);
				dwError = GetLastError();
				if (dwError != ERROR_NO_MORE_FILES)
					LogError(TEXT("FindNextFile"), dwError);
				FindClose(hFind);
			}
			free(file);
		}
		if (attrib & FILE_ATTRIBUTE_READONLY) { // Remove the read-only attribute
			SetFileAttributes(f, attrib&!FILE_ATTRIBUTE_READONLY);
		}
	}
}

// Finds files recursively
// Committed is if the current recursive path only contains files to be deleted (and thus simply list all files found)
// Otherwise wildcards are examined and directories are recursed
static void FindFiles(TCHAR *path, BOOL committed) {
	WIN32_FIND_DATA ffd;
	TCHAR full[BIG_PATH+5];
	HANDLE hFind;
	DWORD dwError, attrib;
	TCHAR *file, *base;
	BOOL match_all;
	set *matches = NULL;

	DWORD len = _tcslen(path);
	while (len > 0 && path[len-1] == L'\\')
		path[--len] = 0;
	file = copyStr(path);
	attrib = GetFileAttributes(path);
	match_all = attrib != INVALID_FILE_ATTRIBUTES && ((attrib & FILE_ATTRIBUTE_DIRECTORY) != 0);
	if (match_all) {
		committed = TRUE;
		file[len  ] = TEXT('\\');
		file[len+1] = TEXT('*');
		file[len+2] = 0;
	} else {
		file[len] = 0;
	}
	
	base = getBaseDirectory(file);
	if (!committed)
		matches = set_create((compare_func)&_tcscmp);

	// first pass we match the pattern only and everything we find is committed
	hFind = FindFirstFileEx(file, FindExInfoBasic, &ffd, FindExSearchNameMatch, NULL, FIND_FIRST_EX_LARGE_FETCH);
	if (hFind == INVALID_HANDLE_VALUE) {
		dwError = GetLastError();
		if (dwError != ERROR_FILE_NOT_FOUND && dwError != ERROR_ACCESS_DENIED)
			LogFileError(TEXT("FindFirstFileEx failed for"), file, dwError);
	} else {
 		do {
			if (_tcscmp(ffd.cFileName, TEXT("..")) == 0 || _tcscmp(ffd.cFileName, TEXT(".")) == 0)
				continue;
			makeFullPath(base, ffd.cFileName, full);
			/*if (ffd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
				_tprintf(TEXT("! Found reparse point '%s': Cannot remove\n"), full);
				continue;
			}*/
			vector_append(files, copyStr(full));
			if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				if (!committed)
					set_insert(matches, copyStr(ffd.cFileName));
				FindFiles(full, TRUE);
			}
		} while (FindNextFile(hFind, &ffd) != 0);
		dwError = GetLastError();
		if (dwError != ERROR_NO_MORE_FILES)
			LogError(TEXT("FindNextFile"), dwError);
		FindClose(hFind);
	}

	if (!committed) {
		// second pass we are just looking for directories to recurse into
		// if we are already committed then there is no need to do this as all directories and files will be enumerated above

		TCHAR *pattern = getFileName(file);
		TCHAR *baseSrch = copyStr(base);
		len = _tcslen(base);
		baseSrch[len ] = L'\\';
		baseSrch[len+1] = L'*';
		baseSrch[len+2] = 0;

		hFind = FindFirstFileEx(baseSrch, FindExInfoBasic, &ffd, FindExSearchLimitToDirectories, NULL, FIND_FIRST_EX_LARGE_FETCH);
		if (hFind == INVALID_HANDLE_VALUE) {
			dwError = GetLastError();
			if (dwError != ERROR_FILE_NOT_FOUND && dwError != ERROR_ACCESS_DENIED)
				LogFileError(TEXT("FindFirstFileEx(!committed) failed for"), baseSrch, dwError);
		} else {
			do {
				if (_tcscmp(ffd.cFileName, TEXT("..")) != 0 && _tcscmp(ffd.cFileName, TEXT(".")) != 0 && ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					if (!set_contains(matches, ffd.cFileName)) { // don't re-recurse into a directory
						makeFullPath2(base, ffd.cFileName, pattern, full);
						/*if (ffd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
							_tprintf(TEXT("! Found reparse point '%s': Cannot remove\n"), full);
						} else {*/
							FindFiles(full, FALSE);
						//}
					}
				}
			} while (FindNextFile(hFind, &ffd) != 0);
			dwError = GetLastError();
			if (dwError != ERROR_NO_MORE_FILES)
				LogError(TEXT("FindNextFile(!committed)"), dwError);
			FindClose(hFind);
		}
		free(baseSrch);
	}

	free(file);
	free(base);
	if (!committed)
		set_destroy(matches, TRUE);
}

// Deletes a file using the shell (SHFileOperation)
// This is required as sometimes RemoveDirectory (and rm) report access is denied but the shell can delete it
static int DeleteWithSH(const TCHAR *f) {
	int retval;
	SHFILEOPSTRUCT op;
	ZeroMemory(&op, sizeof(SHFILEOPSTRUCT));
	op.wFunc = FO_DELETE;
	op.fFlags = FOF_NO_UI;
	op.pFrom = copyStr(f); // needs to be double null terminated (taken care of by copyStr)
	retval = SHFileOperation(&op);
	free((LPVOID)op.pFrom);
	return retval;
}


#include "mingw-unicode.c"
int _tmain(int argc, _TCHAR* argv[]) {
	int i;
	DWORD failures = 0;
	ULONGLONG size = 0;
	DWORD err;
	TCHAR *f;
	WIN32_FILE_ATTRIBUTE_DATA attrib;
	TCHAR sSize[64];

	PSID sid = NULL;
	PACL acl = NULL;
	BOOL takeownership;
	EXPLICIT_ACCESS ea[1];

	if (argc < 2) {
		_tprintf(TEXT("You need to provide at least one file, directory, or wildcard pattern to delete\n"));
		return 1;
	}

	takeownership = EnableTakeOwnershipPriv();
	if (!takeownership) {
		_ftprintf(stderr, TEXT("! Failed to enable the Take Ownership privilege\n"));
		_ftprintf(stderr, TEXT("Make sure you are an Administrator or elevated\n"));
		_ftprintf(stderr, TEXT("We will still try to delete the files\n"));
	}
	sid = GetCurrentSID();
	if (!sid) {
		_ftprintf(stderr, TEXT("! Failed to get the current user's SID\n"));
		_ftprintf(stderr, TEXT("We will still try to delete the files\n"));
	} else {
		// Explicit Access: All Access
		ZeroMemory(&ea, 1*sizeof(EXPLICIT_ACCESS));
		ea[0].grfAccessPermissions = GENERIC_ALL;
		ea[0].grfAccessMode = SET_ACCESS;
		ea[0].grfInheritance = SUB_OBJECTS_ONLY_INHERIT;
		ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
		ea[0].Trustee.TrusteeType = TRUSTEE_IS_USER;
		ea[0].Trustee.ptstrName = (LPTSTR)sid;
		// Create the ACL
		err = SetEntriesInAcl(1, ea, NULL, &acl);
		if (err != ERROR_SUCCESS) {
			LogError(TEXT("SetEntriesInAcl"), err);
		}
	}

	files = vector_create(64);

	// Find all files to delete
	for (i = 1; i < argc; i++) {
		DWORD attrib = GetFileAttributes(argv[i]);
		if (attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY) != 0) {
			/*if (attrib & FILE_ATTRIBUTE_REPARSE_POINT) {
				_tprintf(TEXT("! Found reparse point '%s': Cannot remove\n"), argv[i]);
				continue;
			}*/
			vector_append(files, copyStr(argv[i]));
		}
		FindFiles(argv[i], FALSE);
	}

	// Leave now if there is nothing to delete
	if (files->length == 0) {
		_tprintf(TEXT("No files found\n"));
		vector_destroy(files, TRUE);
		return 0;
	}

	// Correct security and delete files / directories
	for (i = files->length-1; i >= 0; i--) {
		f = files->x[i];
		if (!GetFileAttributesEx(f, GetFileExInfoStandard, &attrib))
			continue;
		CorrectSecurity(f, attrib.dwFileAttributes, takeownership, sid, acl);
		if ((attrib.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
			if (!RemoveDirectory(f) && (err = DeleteWithSH(f)) != 0) {
				LogFileError(TEXT("Failed to delete folder"), f, err);
				failures++;
			}
		} else {
			if (!DeleteFile(f) && (err = DeleteWithSH(f)) != 0) {
				LogFileError(TEXT("Failed to delete file"), f, GetLastError());
				failures++;
			} else {
				size += attrib.nFileSizeLow;
				size += ((ULONGLONG)attrib.nFileSizeHigh) << 32;
			}
		}
	}

	// Show results
	if (size < 1000)			_sntprintf(sSize, 64, TEXT("%I64d bytes"), size);
	else if (size < 1000000)	_sntprintf(sSize, 64, TEXT("%.2f KB"), size / 1024.0);
	else if (size < 1000000000)	_sntprintf(sSize, 64, TEXT("%.2f MB"), size / 1024 / 1024.0);
	else						_sntprintf(sSize, 64, TEXT("%.2f GB"), size / 1024 / 1024 / 1024.0);
	_tprintf(TEXT("Deleted %d files (%s)\n"), files->length-failures, sSize);
	if (failures > 0)
		_tprintf(TEXT("Failed to delete %d files\n"), failures);

	// Cleanup
	vector_destroy(files, TRUE);
	LocalFree(acl);
	LocalFree(sid);

	return 0;
}
