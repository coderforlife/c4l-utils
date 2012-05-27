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

#ifndef _SIGNER_H_
#define _SIGNER_H_

// The trusted root certificate authority store name, where all CAs will be placed
const static WCHAR *Root = L"Root";

// The trusted publisher store name, where all code-signing will be placed
const static WCHAR *TrustedPublisher = L"TrustedPublisher";

// Open a certificate store with the given name. If machine is true, accesses the local machine, otherwise the current user.
static inline HCERTSTORE OpenStore(const WCHAR *name, BOOL machine) {
	return CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0, machine ? CERT_SYSTEM_STORE_LOCAL_MACHINE : CERT_SYSTEM_STORE_CURRENT_USER , name);
}

// Cleans up used resources, by freeing a certificate context and closing the store
static inline void Cleanup(HCERTSTORE store, PCCERT_CONTEXT cert) {
	if (cert)	CertFreeCertificateContext(cert);
	if (store)	CertCloseStore(store, 0);
}

// Checks if a certificate has a private key available on this computer
static inline BOOL HasPrivateKey(PCCERT_CONTEXT cert) {
	return CryptFindCertificateKeyProvInfo(cert, CRYPT_ACQUIRE_ALLOW_NCRYPT_KEY_FLAG, NULL);
}

// Creates a certificate
PCCERT_CONTEXT CreateCert(LPCWSTR name, BOOL machine, HCERTSTORE hCertStore);
// Finds a certificate, possibly requiring that certificate to have a private key available
PCCERT_CONTEXT FindCert(LPCWSTR name, BOOL hasPrivateKey, HCERTSTORE hCertStore);
// Find a certificate in the appropriate store
int FindCertInStore(const WCHAR *name, BOOL ca, BOOL machine, PCCERT_CONTEXT *cert, HCERTSTORE *hCertStore);
// Finds a certificate, or if that fails, create the certificate
static inline PCCERT_CONTEXT FindOrCreateCert(LPCWSTR name, BOOL machine, BOOL hasPrivateKey, HCERTSTORE hCertStore) {
	PCCERT_CONTEXT cert = FindCert(name, hasPrivateKey, hCertStore);
	return cert ? cert : CreateCert(name, machine, hCertStore);
}

// Prepare to sign - call once before any calls to Sign
BOOL PrepareToSign();
// Sign a file with a certificate, the file should be a PE file (exe, dll, sys, ocx, mui) or cat file
BOOL Sign(const WCHAR *file, PCCERT_CONTEXT cert, HCERTSTORE hCertStore);

// Export a certificate
DWORD Export(PCCERT_CONTEXT cert, WCHAR *file);

// Prepare to install - call once before any calls to Install
BOOL PrepareToInstall();
// Install a certificate into an off-line registry store
int Install(const WCHAR *file, const WCHAR *store, PCCERT_CONTEXT cert);

#endif
