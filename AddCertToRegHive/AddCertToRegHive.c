#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_SWPRINTFS

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>

#include <Wincrypt.h>
#ifdef _MSC_VER
#pragma comment(lib, "Crypt32.lib")
#endif

#ifndef CERT_STORE_PROV_REG
#define CERT_STORE_PROV_REG				((LPCSTR)4)
#endif
#ifndef CERT_STORE_BACKUP_RESTORE_FLAG
#define CERT_STORE_BACKUP_RESTORE_FLAG	0x00000800
#endif
#ifndef CERT_STORE_OPEN_EXISTING_FLAG
#define CERT_STORE_OPEN_EXISTING_FLAG	0x00004000
#endif
#ifndef CERT_CLOSE_STORE_FORCE_FLAG
#define CERT_CLOSE_STORE_FORCE_FLAG		0x00000001
#endif

#include <stdio.h>
#include <tchar.h>

#define MAX_REG_KEY_LEN	256

static int Error(LPCTSTR str, LPCTSTR s, DWORD err) {
	if (s == NULL) { _ftprintf(stderr, str); }
	else { _ftprintf(stderr, str, s); }
	_ftprintf(stderr, TEXT(": %u\n\n"), err);
	return err;
}
static int LastError(LPCTSTR str, LPCTSTR s) { return Error(str, s, GetLastError()); }

// Enables privileges by name
static BOOL EnablePriv(LPWSTR priv) {
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

// Read an entire file
// Return value needs to be freed with LocalFree
static LPVOID Read(LPCTSTR file, DWORD *size) {
	DWORD dwRead = 0;
	LPVOID data = NULL;
	HANDLE f = CreateFile(file, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (f == INVALID_HANDLE_VALUE) { return NULL; }
	*size = GetFileSize(f, NULL);
	if (*size != INVALID_FILE_SIZE) {
		data = LocalAlloc(0, *size);
		if (!ReadFile(f, data, *size, &dwRead, NULL) || *size != dwRead) {
			data = LocalFree(data);
		}
	}
	CloseHandle(f);
	return data;
}

typedef struct _Params {
	TCHAR *hive, *cert, *crl, *store;
} Params;

#define IS_CMD(a)		(a[0] == '-' || a[0] == '/')
#define CHK_CMD(a, c)	(wcsicmp(a+1, c) == 0)

BOOL GetParams(Params *p, int argc, TCHAR* argv[]) {
	int i;
	BOOL hive = FALSE, cert = FALSE;

	if (argc < 3) return FALSE;

	p->crl = NULL;
	p->store = TEXT("ROOT");

	for (i = 1; i < argc; ++i) {
		if (!IS_CMD(argv[i])) {
			if (!hive) { p->hive = argv[i]; hive = TRUE; }
			else if (!cert) { p->cert = argv[i]; cert = TRUE; }
			else { return FALSE; } // too many params
		} else if (CHK_CMD(argv[i], TEXT("CRL")) && i < argc-1) {
			p->crl = argv[++i];
		} else if (CHK_CMD(argv[i], TEXT("Store")) && i < argc-1) {
			p->store = argv[++i];
		} else {
			return FALSE;
		}
	}

	return hive && cert;
}

int _tmain(int argc, _TCHAR* argv[]) {
	LONG retval;
	Params p;
	LPVOID cert, crl;
	DWORD certSz = 0, crlSz = 0, index = 0, disp = 0;
	HKEY rootKey = NULL, storesKey = NULL, key = NULL;

	HCERTSTORE hCertStore = NULL;
	TCHAR root[MAX_REG_KEY_LEN];

	// Get params
	if (!GetParams(&p, argc, argv)) {
		_tprintf(TEXT("Usage:\n"));
		_tprintf(TEXT("%s hive crt.cer [/CRL crl.crl] [/Store store]\n\n"), argv[0]);
		_tprintf(TEXT("hive\ta registry hive for HKLM\\SOFTWARE (user hives not supported)\n"));
		_tprintf(TEXT("  found at Windows\\System32\\config\\SOFTWARE (cannot use be an in-use hive)\n"));
		_tprintf(TEXT("crt.cer\tthe certificate to import\n"));
		_tprintf(TEXT("crl.crl\tif provided adds a CRL as well\n"));
		_tprintf(TEXT("store\tthe store to import to, defaults to ROOT\n\n"));
		return -1;
	}

	// Enable privileges
	if (!EnablePriv(SE_TAKE_OWNERSHIP_NAME) || !EnablePriv(SE_BACKUP_NAME) || !EnablePriv(SE_RESTORE_NAME)) {
		return LastError(TEXT("Failed to enable take ownership, backup, and restore privileges"), NULL);
	}

	// Read the certificate file
	if ((cert = Read(p.cert, &certSz)) == NULL) {
		return LastError(TEXT("Failed to read certificate file '%s'"), p.cert);
	}

	// Read the CRL file
	if (p.crl && ((crl = Read(p.crl, &crlSz)) == NULL)) {
		LocalFree(cert);
		return LastError(TEXT("Failed to read the CRL file '%s'"), p.crl);
	}

	// Find a subkey that's available
	_tcsncpy(root, TEXT("TEMPHIVE"), MAX_REG_KEY_LEN);
	if ((retval = RegOpenKeyEx(HKEY_LOCAL_MACHINE, root, 0, KEY_READ, &key)) != ERROR_FILE_NOT_FOUND) {
		if (retval != ERROR_SUCCESS) {
			LocalFree(crl);
			LocalFree(cert);
			return Error(TEXT("Failed to find subkey to load hive"), NULL, retval);
		}
		RegCloseKey(key);
		_sntprintf(root, MAX_REG_KEY_LEN, TEXT("TEMPHIVE%u"), index++);
	}
	key = NULL;

	// Load the hive
	if ((retval = RegLoadKey(HKEY_LOCAL_MACHINE, root, p.hive)) != ERROR_SUCCESS) {
		LocalFree(cert);
		if (crl) LocalFree(crl);
		return Error(TEXT("Failed to load hive file '%s'"), p.hive, retval);
	}

	// Open the HKLM\TEMPHIVE\Microsoft\SystemCertificates
	if ((retval = RegOpenKeyEx(HKEY_LOCAL_MACHINE, root, 0, KEY_ALL_ACCESS, &rootKey)) != ERROR_SUCCESS) {
		Error(TEXT("Failed to get root key '%s'"), root, retval);
	} else if ((retval = RegOpenKeyEx(rootKey, TEXT("Microsoft\\SystemCertificates"), 0, KEY_ALL_ACCESS, &storesKey)) != ERROR_SUCCESS) {
		Error(TEXT("Failed to get stores key: %u\n"), NULL, retval);

	// Create/Open the registry certificate store
	} else if ((retval = RegCreateKeyEx(storesKey, p.store, 0, NULL, REG_OPTION_BACKUP_RESTORE, KEY_ALL_ACCESS, NULL, &key, &disp)) != ERROR_SUCCESS) {
		Error(TEXT("Failed to create store key '%s'"), p.store, retval);

	// Open the store
	} else if ((hCertStore = CertOpenStore(CERT_STORE_PROV_REG, 0, (HCRYPTPROV)NULL, CERT_STORE_BACKUP_RESTORE_FLAG | CERT_STORE_OPEN_EXISTING_FLAG, key)) == NULL) {
		retval = LastError(TEXT("Failed to create certificate store"), NULL);

	// Add the certificate to the store
	} else if (!CertAddEncodedCertificateToStore(hCertStore, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, cert, certSz, CERT_STORE_ADD_REPLACE_EXISTING, NULL)) {
		retval = LastError(TEXT("Failed add certificate to store"), NULL);

	// Add the crl to the store
	} else if (crl && !CertAddEncodedCRLToStore(hCertStore, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, crl, crlSz, CERT_STORE_ADD_REPLACE_EXISTING, NULL)) {
		retval = LastError(TEXT("Failed add the CRL to store"), NULL);
	}

	// Cleanup
	if (hCertStore) { CertCloseStore(hCertStore, CERT_CLOSE_STORE_FORCE_FLAG); }
	if (key)		{ RegCloseKey(key); }
	if (storesKey)	{ RegCloseKey(storesKey); }
	if (rootKey)	{ RegCloseKey(rootKey); }
	LocalFree(crl);
	LocalFree(cert);

	// Unload the hive
	if ((disp = RegUnLoadKey(HKEY_LOCAL_MACHINE, root)) != ERROR_SUCCESS) {
		if (retval == ERROR_SUCCESS) { retval = disp; }
		Error(TEXT("Failed to unload the hive"), NULL, disp);
	}

	// Successful? Yeah!
	if (retval == ERROR_SUCCESS) {
		if (p.crl) {
			_tprintf(TEXT("Successfully added %s and %s to the %s store in %s\n\n"), p.cert, p.crl, p.store, p.hive);
		} else {
			_tprintf(TEXT("Successfully added %s to the %s store in %s\n\n"), p.cert, p.store, p.hive);
		}
	}

	return retval;
}

