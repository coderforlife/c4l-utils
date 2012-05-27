/*
    signer: tool for self-signing
    Copyright (C) 2011  Jeffrey Bush <jeff@coderforlife.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/gpl.html>.
*/

// Functions to use certificates, such as create, sign, export, and install
// See signer.h for descriptions

#include "stdafx.h"
#include "SignerSign.h"
#include "signer.h"
#include "errors.h"

static inline SYSTEMTIME GetStartTime() { SYSTEMTIME t; GetSystemTime(&t); return t; }
static inline SYSTEMTIME GetEndTime() { SYSTEMTIME t; GetSystemTime(&t); t.wYear += 30; return t; }

static BOOL EncodeName(const wchar_t *dn, CERT_NAME_BLOB *name) {
	return CertStrToName(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, dn, CERT_X500_NAME_STR | CERT_NAME_STR_CRLF_FLAG | CERT_NAME_STR_NO_PLUS_FLAG | CERT_NAME_STR_NO_QUOTING_FLAG, NULL, name->pbData, &name->cbData, NULL /*error string*/);
}

static BOOL EncodeKeyUsage(BYTE key_usage, CRYPT_DATA_BLOB *encoded) {
	CRYPT_BIT_BLOB usage;
	usage.cbData = 1;
	usage.cUnusedBits = 0;
	usage.pbData = &key_usage;
	return CryptEncodeObject(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, szOID_KEY_USAGE, &usage, encoded->pbData, &encoded->cbData);
}

static BOOL CreateBasicContraints(BOOL ca, BOOL end, CRYPT_DATA_BLOB *encoded) {
	CERT_BASIC_CONSTRAINTS_INFO bc;
	BYTE type_val = (ca ? CERT_CA_SUBJECT_FLAG : 0) | (end ? CERT_END_ENTITY_SUBJECT_FLAG : 0);
	ZeroMemory(&bc, sizeof(bc));
	bc.SubjectType.cbData = 1;
	bc.SubjectType.cUnusedBits = 6;
	bc.SubjectType.pbData = &type_val;
	return CryptEncodeObject(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, szOID_BASIC_CONSTRAINTS, &bc, encoded->pbData, &encoded->cbData);
}

static PCCERT_CONTEXT CreateCA(const wchar_t *dn, BOOL machine) {
	DWORD err;

	SYSTEMTIME start = GetStartTime(), end = GetEndTime();
	
	BYTE name_buffer[256], usage_buffer[8], basic_buffer[32];
	CERT_NAME_BLOB name = { ARRAYSIZE(name_buffer), name_buffer };
	CERT_EXTENSION exts_buffer[] = {
		{ szOID_KEY_USAGE, FALSE, ARRAYSIZE(usage_buffer), usage_buffer },
		{ szOID_BASIC_CONSTRAINTS, TRUE, ARRAYSIZE(basic_buffer), basic_buffer }
	};
	CERT_EXTENSIONS exts = { ARRAYSIZE(exts_buffer), exts_buffer };

	PCCERT_CONTEXT ca, _ca;
	HCERTSTORE hCertStore;

	// Encode the name
	if (!EncodeName(dn, &name)) { /* error! */ return NULL; }

	// Encode the key usage  //CERT_KEY_ENCIPHERMENT_KEY_USAGE | CERT_DATA_ENCIPHERMENT_KEY_USAGE | CERT_KEY_AGREEMENT_KEY_USAGE
	if (!EncodeKeyUsage(CERT_DIGITAL_SIGNATURE_KEY_USAGE | CERT_NON_REPUDIATION_KEY_USAGE | CERT_KEY_CERT_SIGN_KEY_USAGE | CERT_OFFLINE_CRL_SIGN_KEY_USAGE | CERT_CRL_SIGN_KEY_USAGE, &exts.rgExtension[0].Value)) { /* error! */ return NULL; }

	// Encode the basic constraints
	if (!CreateBasicContraints(TRUE, FALSE, &exts.rgExtension[1].Value)) { /* error! */ return NULL; }

	// Create the CA
	if ((ca = CertCreateSelfSignCertificate(0 /*default: RSA_FULL and AT_SIGNATURE*/, &name, 0, NULL, NULL /*default: SHA1RSA*/, &start, &end, &exts)) == NULL) { /* error! */ return NULL; }

	// Open the root store
	if ((hCertStore = OpenStore(Root, machine)) == NULL) { /* error! */ CertFreeCertificateContext(ca); return NULL; }

	// Add the CA to the root
	err = CertAddCertificateContextToStore(hCertStore, ca, CERT_STORE_ADD_USE_EXISTING, &_ca) ? 0 : GetLastError();

	// Cleanup from CA
	CertFreeCertificateContext(ca);
	CertCloseStore(hCertStore, 0); // CERT_CLOSE_STORE_FORCE_FLAG
	if (err) { /* error! */ SetLastError(err); return NULL; }

	return _ca;
}

static PCERT_PUBLIC_KEY_INFO GetPublicKey(const wchar_t *dn) {
	HCRYPTPROV hUserProv = 0;
	HCRYPTKEY hUserKey = 0;

	DWORD err, cbEncodedPubKey = 0;
	PCERT_PUBLIC_KEY_INFO pPubKeyInfo = NULL;

	if (!CryptAcquireContext(&hUserProv, dn, MS_ENHANCED_PROV, PROV_RSA_FULL, CRYPT_NEWKEYSET) &&
		(NTE_EXISTS != GetLastError() || !CryptAcquireContext(&hUserProv, dn, MS_ENHANCED_PROV, PROV_RSA_FULL, 0))) { /* error! */ return NULL; }

	if (!CryptGetUserKey(hUserProv, AT_SIGNATURE, &hUserKey) && !CryptGenKey(hUserProv, AT_SIGNATURE, 0, &hUserKey)) { /* error! */ CryptReleaseContext(hUserProv, 0); return NULL; }
	CryptDestroyKey(hUserKey);

	// Get the public key
	if (CryptExportPublicKeyInfoEx(hUserProv, AT_SIGNATURE, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, szOID_RSA_RSA, 0, NULL, NULL, &cbEncodedPubKey)) {
		pPubKeyInfo = (PCERT_PUBLIC_KEY_INFO) LocalAlloc(0, cbEncodedPubKey);
		err = CryptExportPublicKeyInfoEx(hUserProv, AT_SIGNATURE, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, szOID_RSA_RSA, 0, NULL, pPubKeyInfo, &cbEncodedPubKey) ? 0 : GetLastError();
	}
	CryptReleaseContext(hUserProv, 0);
	if (err) { /* error! */ if (pPubKeyInfo) LocalFree(pPubKeyInfo); SetLastError(err); return NULL; }

	return pPubKeyInfo;
}

static HCRYPTPROV GetProviderFromCert(PCCERT_CONTEXT c) {
	HCRYPTPROV p = 0;
	DWORD l = 0;
	if (CertGetCertificateContextProperty(c, CERT_KEY_PROV_INFO_PROP_ID, NULL, &l)) {
		PCRYPT_KEY_PROV_INFO k = (PCRYPT_KEY_PROV_INFO)LocalAlloc(0, l);
		if (CertGetCertificateContextProperty(c, CERT_KEY_PROV_INFO_PROP_ID, (void*)k, &l)) {
			if (!CryptAcquireContext(&p, k->pwszContainerName, k->pwszProvName, k->dwProvType, k->dwFlags))
				p = 0;
			LocalFree(k);
		}
	}
	return p;
}

static inline void GenerateSerialNumber(LPBYTE data, DWORD count) {
	DWORD i;
	srand((unsigned int)time(NULL));
	for (i = 0; i < count; ++i)
		data[i] = rand() % 0x100;
}

static inline BOOL CreateCodeSigningEnKeyUsage(CRYPT_DATA_BLOB *encoded) {
	LPSTR e_usages[] = { szOID_PKIX_KP_CODE_SIGNING };
	CERT_ENHKEY_USAGE e_usage = { ARRAYSIZE(e_usages), e_usages };
	return CryptEncodeObject(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, X509_ENHANCED_KEY_USAGE, &e_usage, encoded->pbData, &encoded->cbData);
}

static PCCERT_CONTEXT CreateCodeSigningCert(PCCERT_CONTEXT ca, const wchar_t *dn, HCERTSTORE hCertStore) {
	DWORD err;

	SYSTEMTIME start = GetStartTime(), end = GetEndTime();

	BYTE name_buffer[256], usage_buffer[8], e_usage_buffer[256], basic_buffer[32], serial[16];
	CERT_NAME_BLOB name = { ARRAYSIZE(name_buffer), name_buffer };
	CERT_EXTENSION exts[] = {
		{ szOID_KEY_USAGE, FALSE, ARRAYSIZE(usage_buffer), usage_buffer },
		{ szOID_ENHANCED_KEY_USAGE, FALSE, ARRAYSIZE(e_usage_buffer), e_usage_buffer },
		{ szOID_BASIC_CONSTRAINTS, TRUE, ARRAYSIZE(basic_buffer), basic_buffer }
	};

	PCERT_PUBLIC_KEY_INFO pPubKeyInfo;
	CERT_INFO ci;
	CRYPT_ALGORITHM_IDENTIFIER ai;

	HCRYPTPROV hCaProv;
	BYTE cert_buf[1024];
	DWORD cert_len = ARRAYSIZE(cert_buf);

	PCCERT_CONTEXT cert;

	// Encode the name
	if (!EncodeName(dn, &name)) { /* error! */ return NULL; }

	// Encode the key usage
	if (!EncodeKeyUsage(CERT_DIGITAL_SIGNATURE_KEY_USAGE, &exts[0].Value)) { /* error! */ return NULL; }

	// Encode the enhanced key usage
	if (!CreateCodeSigningEnKeyUsage(&exts[1].Value)) { /* error! */ return NULL; }

	// Encode the basic constraints
	if (!CreateBasicContraints(FALSE, TRUE, &exts[2].Value)) { /* error! */ return NULL; }

	// Get basic information
	GenerateSerialNumber(serial, ARRAYSIZE(serial));
	if ((pPubKeyInfo = GetPublicKey(dn)) == NULL) { /* error! */ return NULL; }

	// Build the certificate information
	ZeroMemory(&ci, sizeof(ci));
	ci.dwVersion = CERT_V3;
	ci.SerialNumber.cbData = ARRAYSIZE(serial);
	ci.SerialNumber.pbData = serial;
	ci.SignatureAlgorithm.pszObjId = szOID_RSA_SHA1RSA;
	ci.Issuer = ca->pCertInfo->Subject;
	SystemTimeToFileTime(&start, &ci.NotBefore);
	SystemTimeToFileTime(&end, &ci.NotAfter);
	ci.Subject = name;
	ci.SubjectPublicKeyInfo = *pPubKeyInfo;
	ci.IssuerUniqueId = ca->pCertInfo->SubjectUniqueId;
	//ci.SubjectUniqueId = ; // CRYPT_BIT_BLOB
	ci.cExtension = ARRAYSIZE(exts);
	ci.rgExtension = exts;

	// Build the algorithm information
	ZeroMemory(&ai, sizeof(ai));
	ai.pszObjId = szOID_RSA_SHA1RSA;

	// Get the provider
	if ((hCaProv = GetProviderFromCert(ca)) == 0) { /* error! */ LocalFree(pPubKeyInfo); return NULL; }

	// Sign the certificate
	err = CryptSignAndEncodeCertificate(hCaProv, AT_SIGNATURE, X509_ASN_ENCODING, X509_CERT_TO_BE_SIGNED, (void*)&ci, &ai, NULL, cert_buf, &cert_len) ? 0 : GetLastError();

	// Cleanup a bit
	CryptReleaseContext(hCaProv, 0);
	LocalFree(pPubKeyInfo);
	if (err) { /* error! */ SetLastError(err); return NULL; }

	// Add the certificate to the store, and get the context
	cert;
	if (!CertAddEncodedCertificateToStore(hCertStore, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, cert_buf, cert_len, CERT_STORE_ADD_USE_EXISTING, &cert)) {
		/* error! */
		return NULL;
	}
	
	// Find the private key and associate it with the certificate. This needs to be done, and before the store is closed so it is fully saved.
	if (!HasPrivateKey(cert)) {
		/* error! */
		CertFreeCertificateContext(cert);
		return NULL;
	}
	
	return cert;
}

PCCERT_CONTEXT CreateCert(LPCWSTR name, BOOL machine, HCERTSTORE hCertStore) {
	WCHAR _name[1024];
	PCCERT_CONTEXT ca, cert;

	// Create the CA
	_snwprintf(_name, ARRAYSIZE(_name), L"CN=%s Certificate Authority", name);
	if ((ca = CreateCA(_name, machine)) == NULL) { /* error! */ return NULL; }

	// Create Code Signing Cert
	_snwprintf(_name, ARRAYSIZE(_name), L"CN=%s", name);
	cert = CreateCodeSigningCert(ca, _name, hCertStore);
	CertFreeCertificateContext(ca);
	if (cert == NULL) { /* error! */ return NULL; }
	
	return cert;
}

PCCERT_CONTEXT FindCert(LPCWSTR name, BOOL hasPrivateKey, HCERTSTORE hCertStore) {
	DWORD err;
	WCHAR _name[512];
	PCCERT_CONTEXT cert = NULL;
	while ((cert = CertEnumCertificatesInStore(hCertStore, cert)) != NULL) {
		if (!hasPrivateKey || HasPrivateKey(cert)) {
			if (CertGetNameString(cert, CERT_NAME_FRIENDLY_DISPLAY_TYPE, 0, NULL, _name, ARRAYSIZE(_name))) {
				if (_wcsicmp(name, _name) == 0) {
					// Found!
					return cert;
				}
			} else {
				/* error! */
				last_error(L"Failed to get certificate name");
			}
		}
	}
	err = GetLastError();
	if (err != CRYPT_E_NOT_FOUND && err != ERROR_NO_MORE_FILES) {
		/* error! */
		last_error(L"Failed to fully list certificates");
	}

	return NULL;
}

int FindCertInStore(const WCHAR *name, BOOL ca, BOOL machine, PCCERT_CONTEXT *cert, HCERTSTORE *hCertStore) {
	HCERTSTORE h;
	PCCERT_CONTEXT c;

	WCHAR n[512];
	if (ca)	_snwprintf(n, ARRAYSIZE(n), L"%s Certificate Authority", name);
	else	wcsncpy(n, name, ARRAYSIZE(n));

	if ((h = OpenStore(ca ? Root : TrustedPublisher, machine)) == NULL) return last_error(L"Failed to open store");
	if ((c = FindCert(n, FALSE, h)) == NULL) return last_error_and_close_store(L"Failed to find certificate", h);

	*hCertStore = h;
	*cert = c;
	return 0;
}

BOOL PrepareToSign() {
	HMODULE signer = LoadLibrary(L"mssign32.dll");
	return signer && LOAD_FUNC(SignerSignEx, signer) && LOAD_FUNC(SignerTimeStampEx, signer) && LOAD_FUNC(SignerFreeSignerContext, signer);
}

BOOL Sign(const WCHAR *file, PCCERT_CONTEXT cert, HCERTSTORE hCertStore) {
	DWORD dwIndex = 0;
	SIGNER_FILE_INFO FileInfo = { sizeof(SIGNER_FILE_INFO), file, NULL };
	SIGNER_SUBJECT_INFO SubjectInfo = { sizeof(SIGNER_SUBJECT_INFO), &dwIndex, SIGNER_SUBJECT_FILE, &FileInfo };

	SIGNER_CERT_STORE_INFO StoreInfo = { sizeof(SIGNER_CERT_STORE_INFO), cert, SIGNER_CERT_POLICY_CHAIN, hCertStore };
	SIGNER_CERT SignerCert = { sizeof(SIGNER_CERT), SIGNER_CERT_STORE, &StoreInfo, NULL };

	SIGNER_SIGNATURE_INFO SignatureInfo = { sizeof(SIGNER_SIGNATURE_INFO), CALG_SHA1, SIGNER_NO_ATTR, NULL, NULL, NULL };

	SIGNER_CONTEXT *pSignerContext = NULL;

	HRESULT res = SignerSignEx(0, &SubjectInfo, &SignerCert, &SignatureInfo, NULL, /*ts*/ NULL, NULL, NULL, &pSignerContext);

	if (res == ERROR_SUCCESS) {
		// Timestamping does not work for SignerSignEx, so we have to add the timestamp afterwards
		if (pSignerContext) { SignerFreeSignerContext(pSignerContext); pSignerContext = NULL; }
		res = SignerTimeStampEx(0, &SubjectInfo, L"http://timestamp.verisign.com/scripts/timestamp.dll", NULL, NULL, &pSignerContext);
	}

	if (res != ERROR_SUCCESS) {
		SetLastError(res);
	}

	if (pSignerContext) { SignerFreeSignerContext(pSignerContext); }

	return res == ERROR_SUCCESS;
}

DWORD Export(PCCERT_CONTEXT cert, WCHAR *file) {
	DWORD err = 0;
	DWORD written = 0;
	HANDLE h = CreateFile(file, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (h == INVALID_HANDLE_VALUE)
		err = last_error_str(L"Failed to open file '%s'", file);
	else if (!WriteFile(h, cert->pbCertEncoded, cert->cbCertEncoded, &written, NULL) || written != cert->cbCertEncoded)
		err = last_error_str(L"Failed to completely write file '%s'", file);
	if (h != INVALID_HANDLE_VALUE) CloseHandle(h);
	return err;
}

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

BOOL PrepareToInstall() { return EnablePriv(SE_TAKE_OWNERSHIP_NAME) && EnablePriv(SE_BACKUP_NAME) && EnablePriv(SE_RESTORE_NAME); }

#define MAX_REG_KEY_LEN 255
int Install(const WCHAR *file, const WCHAR *store, PCCERT_CONTEXT cert) {
	LONG retval;
	DWORD index = 0, disp = 0;
	WCHAR path[MAX_REG_KEY_LEN+1];
	HKEY root, stores, key = NULL;
	HCERTSTORE hCertStore;

	// Find a subkey that's available
	wcsncpy(path, L"TEMPHIVE", MAX_REG_KEY_LEN);
	while ((retval = RegOpenKeyEx(HKEY_LOCAL_MACHINE, path, 0, KEY_READ, &key)) != ERROR_FILE_NOT_FOUND) {
		if (retval != ERROR_SUCCESS) return error(L"Failed to find subkey to load hive", retval);
		RegCloseKey(key);
		_snwprintf(path, MAX_REG_KEY_LEN, L"TEMPHIVE%u", index++);
	}
	key = NULL;

	// Load the hive
	if ((retval = RegLoadKey(HKEY_LOCAL_MACHINE, path, file)) != ERROR_SUCCESS)
		return error(L"Failed to load hive file", retval);

	// Open the HKLM\TEMPHIVE\Microsoft\SystemCertificates
	if ((retval = RegOpenKeyEx(HKEY_LOCAL_MACHINE, path, 0, KEY_ALL_ACCESS, &root)) != ERROR_SUCCESS) {
		retval = error(L"Failed to get root key", retval);
	} else if (((retval = RegOpenKeyEx(root, L"Microsoft\\SystemCertificates", 0, KEY_ALL_ACCESS, &stores)) != ERROR_SUCCESS) && // if this is a local machine hive
		       ((retval = RegOpenKeyEx(root, L"SOFTWARE\\Microsoft\\SystemCertificates", 0, KEY_ALL_ACCESS, &stores)) != ERROR_SUCCESS)) { // if this is a user hive
		retval = error(L"Failed to get stores key", retval);
	// Create/Open the registry certificate store
	} else if ((retval = RegCreateKeyEx(stores, store, 0, NULL, REG_OPTION_BACKUP_RESTORE, KEY_ALL_ACCESS, NULL, &key, &disp)) != ERROR_SUCCESS) {
		retval = error(L"Failed to create store key", retval);
	// Open the store
	} else if ((hCertStore = CertOpenStore(CERT_STORE_PROV_REG, 0, 0, CERT_STORE_BACKUP_RESTORE_FLAG | CERT_STORE_OPEN_EXISTING_FLAG, key)) == NULL) {
		retval = last_error(L"Failed to create certificate store");
	// Add the certificate to the store
	} else if (!CertAddCertificateContextToStore(hCertStore, cert, CERT_STORE_ADD_REPLACE_EXISTING, NULL)) {
		retval = last_error(L"Failed add certificate to store");
	}

	if (hCertStore) { CertCloseStore(hCertStore, CERT_CLOSE_STORE_FORCE_FLAG); }
	if (key)		RegCloseKey(key);
	if (stores)		RegCloseKey(stores);
	if (root)		RegCloseKey(root);

	// Unload the hive
	if ((disp = RegUnLoadKey(HKEY_LOCAL_MACHINE, path)) != ERROR_SUCCESS) {
		if (retval == ERROR_SUCCESS) { retval = disp; }
		error(L"Failed to unload the hive", disp);
	}

	return retval;
}
