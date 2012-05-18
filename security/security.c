#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_SWPRINTFS

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>

#include <Aclapi.h>
#include <Sddl.h>		// for ConvertSidToStringSid (debugging)

#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>

static int error(LPCTSTR s, DWORD err) {
	_ftprintf(stderr, s, GetLastError());
	return err;
}

static int errFile(LPCTSTR s, LPCTSTR file, DWORD err) {
	_ftprintf(stderr, s, GetLastError(), file);
	return err;
}

// Enable privileges
static BOOL EnablePriv(LPCTSTR priv) {
	HANDLE hToken;
	LUID luid;
	TOKEN_PRIVILEGES tkprivs;
	BOOL retval = FALSE;
	ZeroMemory(&tkprivs, sizeof(tkprivs));
	if (!OpenProcessToken(GetCurrentProcess(), (TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY), &hToken)) { return FALSE; }
	if (LookupPrivilegeValue(NULL, priv, &luid)){
		tkprivs.PrivilegeCount = 1;
		tkprivs.Privileges[0].Luid = luid;
		tkprivs.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		retval = AdjustTokenPrivileges(hToken, FALSE, &tkprivs, sizeof(tkprivs), NULL, NULL)==TRUE;
	}
	CloseHandle(hToken);
	return retval;
}

// Enable the SeTakeOwnership and SeRestorePrivilege privileges
// SeTakeOwnership allows a file to become under the ownership of the current user
// SeRestorePrivilege allows a file to be put under the ownership of a different user
static BOOL EnableTakeOwnershipPriv() {
	return EnablePriv(SE_TAKE_OWNERSHIP_NAME) && EnablePriv(SE_RESTORE_NAME);
}

// Extracts the security from a file and saves it as a binary file
// Only extracts owner and DACL
static int ExtractSecurity(LPCTSTR file, LPCTSTR secFile) {
	DWORD retval = 0, size;
	LPVOID sec;
	// Get the size of the security information
	if (!GetFileSecurity(file, OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION, NULL, 0, &size) && GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
		return errFile(TEXT("Failed to get security of the file: %u (%s)\n"), file, 10);
	}
	// Get the security information
	sec = LocalAlloc(LMEM_ZEROINIT, size);
	if (!GetFileSecurity(file, OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION, sec, size, &size)) {
		retval = errFile(TEXT("Failed to get security of the file: %u (%s)\n"), file, 11);
	} else {
		// Save the security information
		HANDLE f = CreateFile(secFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (f == INVALID_HANDLE_VALUE) { retval = errFile(TEXT("Failed to open the security file: %u (%s)\n"), secFile, 12); }
		else {
			if (!WriteFile(f, sec, size, &size, NULL)) { retval = errFile(TEXT("Failed to save the security file: %u (%s)\n"), secFile, 13); }
			CloseHandle(f);
		}
	}
	LocalFree(sec);
	return retval;
}

// Applies the security from a binary file saved with extract to a given file
// Only applies owner and DACL
static int ApplySecurity(LPCTSTR file, LPCTSTR secFile) {
	HANDLE f;
	DWORD retval = 0, size = 0, dwRead = 0;
	LPVOID data = NULL;

	// Read in the security info
	f = CreateFile(secFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (f == INVALID_HANDLE_VALUE)							{ return errFile(TEXT("Failed to open security info file: %u (%s)\n"), secFile, 1); }
	if ((size = GetFileSize(f, NULL)) == INVALID_FILE_SIZE)	{ retval = errFile(TEXT("Failed to get security info file size: %u (%s)\n"), secFile, 2); }
	else if ((data=LocalAlloc(LMEM_ZEROINIT, size))==NULL)	{ retval = error(TEXT("Failed to alloc security info memory: %u\n"), 3); }
	else if (!ReadFile(f, data, size, &dwRead, NULL))		{ retval = errFile(TEXT("Failed to read security info file: %u (%s)\n"), secFile, 4); }
	else if (size != dwRead)								{ retval = errFile(TEXT("Failed to completely read security info file: %u (%s)\n"), secFile, 5); }
	CloseHandle(f);
	if (retval != 0) { if (data) LocalFree(data); return retval; }

	// Apply the security info
	if (!SetFileSecurity(file, OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION, data)) {
		retval = errFile(TEXT("Failed to apply security info to file: %u (%s)\n"), file, 6);
	}
	LocalFree(data);

	return retval;
}

static BOOL checkCmd(LPCTSTR cmd, LPCTSTR arg) {
	return (arg[0] == TEXT('/') || arg[0] == TEXT('-')) && (_tcsicmp(cmd, arg+1) == 0);
}

int _tmain(int argc, _TCHAR* argv[]) {
	DWORD retval = -2;

	if (argc != 4 || (argc == 4 && !checkCmd(TEXT("extract"), argv[1]) && !checkCmd(TEXT("apply"), argv[1]))) {
		_tprintf(TEXT("Usage:\n"));
		_tprintf(TEXT("%s /extract file secinfo.bin\n"), argv[0]);
		_tprintf(TEXT("%s /apply secinfo.bin file\n"), argv[0]);
		return -1;
	}

	if (checkCmd(TEXT("extract"), argv[1])) {
		retval = ExtractSecurity(argv[2], argv[3]);
	} else if (checkCmd(TEXT("apply"), argv[1])) {
		if (!EnableTakeOwnershipPriv()) { return error(TEXT("Failed to enable take ownership privilege: %u\n"), -3); }
		retval = ApplySecurity(argv[3], argv[2]);
	}

	return retval;
}
