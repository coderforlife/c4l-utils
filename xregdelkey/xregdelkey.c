#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_SWPRINTFS

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#include <Aclapi.h>
//#include <Sddl.h> // for SID->String conversion (debugging)

#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

static BOOL LogError(TCHAR *s, DWORD err) { _ftprintf(stderr, TEXT("! %s: %u\n"), s, err); return FALSE; }
static BOOL LogLastError(TCHAR *s) { return LogError(s, GetLastError()); }
#define DO(x) (retval = x) == ERROR_SUCCESS

// The names of the root keys, without the HK since they all start with that
const static TCHAR* rootKeyNames[] = {
	TEXT("EY_CLASSES_ROOT"),	TEXT("CR"),
	TEXT("EY_CURRENT_USER"),	TEXT("CU"),
	TEXT("EY_LOCAL_MACHINE"),	TEXT("LM"),
	TEXT("EY_USERS"),			TEXT("U"),
	TEXT("EY_CURRENT_CONFIG"),	TEXT("CC"),
	NULL
};

// The actually HKEY objects for the names above
const static HKEY rootKeys[] = {
	HKEY_CLASSES_ROOT,	HKEY_CLASSES_ROOT,
	HKEY_CURRENT_USER,	HKEY_CURRENT_USER,
	HKEY_LOCAL_MACHINE,	HKEY_LOCAL_MACHINE,
	HKEY_USERS,			HKEY_USERS,
	HKEY_CURRENT_CONFIG,HKEY_CURRENT_CONFIG
};

// Gets the root key out of the the full name of the string
static HKEY GetRootKey(TCHAR *key, TCHAR **subkey) {
	int i = 0, j;
	if (key[0] != TEXT('H') || key[1] != TEXT('K')) {
		return NULL;
	}
	key += 2; // skip HK
	// find position of '\'
	while (key[i] && key[i] != TEXT('\\')) ++i;
	if (!key[i]) {
		return NULL; // cannot delete root keys
	}
	// find the actually HKEY object from our little lookup table
	for (j = 0; rootKeyNames[j]; ++j) {
		if (_tcsnicmp(key, rootKeyNames[j], i) == 0) {
			*subkey = &key[i+1];
			return rootKeys[j];
		}
	}
	// not found
	return NULL;
}

// Attempts to delete a registry key and all of its subkeys recursively
// Returns ERROR_SUCCESS on success, otherwise returns and error
static LONG _DeleteRegKey(HKEY root, TCHAR *subkey) {
	HKEY hKey;
	LONG retval;
	DWORD i = 0, size = 256;
	TCHAR name[256];
	if (!DO(RegOpenKeyEx(root, subkey, 0, DELETE | KEY_ENUMERATE_SUB_KEYS, &hKey))) {
		if (retval != ERROR_ACCESS_DENIED && retval != ERROR_FILE_NOT_FOUND) {
			LogError(TEXT("RegOpenKeyEx"), retval);
		}
		return retval;
	}
	while (DO(RegEnumKeyEx(hKey, i, name, &size, NULL, NULL, NULL, NULL))) {
		if (!DO(_DeleteRegKey(hKey, name))) { break; }
		++i;
		size = 256;
	}
	if (retval == ERROR_NO_MORE_ITEMS) {
		retval = RegDeleteKey(root, subkey);
		if (retval != ERROR_SUCCESS) {
			LogError(TEXT("RegDeleteKey"), retval);
		}
	} else {
		LogError(TEXT("RegEnumKeyEx"), retval);
	}
	RegCloseKey(hKey);
	return retval;
}

// Deletes a registry key and its subkeys
// If sid is provided, that user is granted full access to key first
// If take_ownship is TRUE, the ownership changed to the user described by sid
// Return TRUE if successfully deleted or did not exist, FALSE otherwise
static BOOL DeleteRegKey(TCHAR *key, PSID sid, BOOL take_ownership) {
	TCHAR *subkey = NULL;
	HKEY root = GetRootKey(key, &subkey);
	HKEY hKey;
	SECURITY_DESCRIPTOR sd;
	LONG retval;
	if (!root) {
		int i;
		_ftprintf(stderr, TEXT("! Could not understand the root key. It must be one of:\n\tHK%s"), rootKeyNames[0]);
		for (i = 1; rootKeyNames[i]; ++i) { // print out list of root HKEYs that we have
			_ftprintf(stderr, TEXT(", HK%s"), rootKeyNames[i]);
		}
		_ftprintf(stderr, TEXT("\n"));
		return FALSE;
	}
	// Just try to delete it before we do anything else, it could turn out we already have the right to
	retval = _DeleteRegKey(root, subkey);
	if (retval == ERROR_SUCCESS) {
		return TRUE;
	} else if (retval == ERROR_FILE_NOT_FOUND) {
		return TRUE; // already deleted
	} else if (retval != ERROR_ACCESS_DENIED) {
		return FALSE; // error we can't fix
	}
	// Take ownsership
	if (sid && take_ownership) {
		if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))	{ return LogLastError(TEXT("InitializeSecurityDescriptor")); }
		if (!DO(RegOpenKeyEx(root, subkey, 0, WRITE_OWNER, &hKey)))				{ return LogError(TEXT("RegOpenKeyEx"), retval); }
		sd.Owner = sid; // sets the owner
		if (!DO(RegSetKeySecurity(hKey, OWNER_SECURITY_INFORMATION, &sd)))		{ return LogError(TEXT("RegSetKeySecurity\n"), retval); }
		if (!DO(RegCloseKey(hKey)))												{ return LogError(TEXT("RegCloseKey\n"), retval); }
	}
	// Grant full access
	if (sid) {
		EXPLICIT_ACCESS ea[1];
		PACL acl = NULL;
		if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))	{ return LogLastError(TEXT("InitializeSecurityDescriptor\n")); }
		if (!DO(RegOpenKeyEx(root, subkey, 0, WRITE_DAC, &hKey)))				{ return LogError(TEXT("RegOpenKeyEx\n"), retval); }
		// Explict Access: All Access
		ZeroMemory(&ea, 1*sizeof(EXPLICIT_ACCESS));
		ea[0].grfAccessPermissions = GENERIC_ALL;
		ea[0].grfAccessMode = SET_ACCESS;
		ea[0].grfInheritance = SUB_OBJECTS_ONLY_INHERIT;
		ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
		ea[0].Trustee.TrusteeType = TRUSTEE_IS_USER;
		ea[0].Trustee.ptstrName = (LPTSTR)sid;
		// Create the ACL and add it to the security descriptor
		if (!DO(SetEntriesInAcl(1, ea, NULL, &acl)))							{ return LogError(TEXT("SetEntriesInAcl\n"), retval); }
		if (!SetSecurityDescriptorDacl(&sd, TRUE, acl, FALSE))					{ LocalFree(acl); return LogLastError(TEXT("SetSecurityDescriptorDacl\n")); }
		retval = RegSetKeySecurity(hKey, DACL_SECURITY_INFORMATION, &sd);
		LocalFree(acl);
		if (retval != ERROR_SUCCESS)											{ return LogError(TEXT("RegSetKeySecurity\n"), retval); }
		if (!DO(RegCloseKey(hKey)))												{ return LogError(TEXT("RegCloseKey\n"), retval); }
	}
	retval = _DeleteRegKey(root, subkey);
	if (retval == ERROR_ACCESS_DENIED) {
		_ftprintf(stderr, TEXT("! Access is still denied...\n"));
	}
	return retval == ERROR_SUCCESS;
}

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

#include "mingw-unicode.c"
int _tmain(int argc, _TCHAR* argv[]) {
	BOOL take_ownership;
	PSID sid = NULL;
	int i;
	if (argc < 2) {
		_tprintf(TEXT("Usage: %s key1 [key2 ...]\n"), argv[0]);
		return 1;
	}
	// Enable taking ownership
	take_ownership = EnableTakeOwnershipPriv();
	if (!take_ownership) {
		_ftprintf(stderr, TEXT("# Failed to obtain the Take Ownership privilege\n"));
		_ftprintf(stderr, TEXT("# We will try to delete anyways.\n\n"));
	}
	// Get the current user's SID
	sid = GetCurrentSID();
	if (!sid) {
		_ftprintf(stderr, TEXT("# Failed to get the current user SID\n"));
		_ftprintf(stderr, TEXT("# We will try to delete anyways.\n\n"));
	}
	// Delete all the keys
	for (i = 1; i < argc; i++) {
		if (!DeleteRegKey(argv[i], sid, take_ownership)) {
			_ftprintf(stderr, TEXT("# Failed to delete '%s'\n"), argv[i]);
		} else {
			_tprintf(TEXT("Deleted '%s'\n"), argv[i]);
		}
	}
	LocalFree(sid);
	return 0;
}

