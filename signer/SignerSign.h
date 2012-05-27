#ifndef _SIGNER_SIGN_H_
#define _SIGNER_SIGN_H_

// The declarations for the SignerSign family of functions.
// Taken directly from the MSDN, see http://msdn.microsoft.com/en-us/library/aa387734(VS.85).aspx

#define SPC_EXC_PE_PAGE_HASHES_FLAG			0x10
#define SPC_INC_PE_IMPORT_ADDR_TABLE_FLAG	0x20
#define SPC_INC_PE_DEBUG_INFO_FLAG			0x40
#define SPC_INC_PE_RESOURCES_FLAG			0x80
#define SPC_INC_PE_PAGE_HASHES_FLAG			0x100

typedef struct _SIGNER_FILE_INFO {
	DWORD   cbSize;
	LPCWSTR pwszFileName;
	HANDLE  hFile;
} SIGNER_FILE_INFO, *PSIGNER_FILE_INFO;

typedef struct _SIGNER_BLOB_INFO {
	DWORD   cbSize;
	GUID    *pGuidSubject;
	DWORD   cbBlob;
	BYTE    *pbBlob;
	LPCWSTR pwszDisplayName;
} SIGNER_BLOB_INFO, *PSIGNER_BLOB_INFO;

#define SIGNER_SUBJECT_BLOB		2
#define SIGNER_SUBJECT_FILE		1

typedef struct _SIGNER_SUBJECT_INFO {
	DWORD cbSize;
	DWORD *pdwIndex;
	DWORD dwSubjectChoice;
	union {
		SIGNER_FILE_INFO *pSignerFileInfo;
//		SIGNER_BLOB_INFO *pSignerBlobInfo;
	} ;
} SIGNER_SUBJECT_INFO, *PSIGNER_SUBJECT_INFO;

#define SIGNER_CERT_POLICY_CHAIN			2
#define SIGNER_CERT_POLICY_CHAIN_NO_ROOT	8
#define SIGNER_CERT_POLICY_STORE			1

typedef struct _SIGNER_CERT_STORE_INFO {
	DWORD          cbSize;
	PCCERT_CONTEXT pSigningCert;
	DWORD          dwCertPolicy;
	HCERTSTORE     hCertStore;
} SIGNER_CERT_STORE_INFO, *PSIGNER_CERT_STORE_INFO;

typedef struct _SIGNER_SPC_CHAIN_INFO {
	DWORD      cbSize;
	LPCWSTR    pwszSpcFile;
	DWORD      dwCertPolicy;
	HCERTSTORE hCertStore;
} SIGNER_SPC_CHAIN_INFO, *PSIGNER_SPC_CHAIN_INFO;

#define SIGNER_CERT_SPC_FILE	1
#define SIGNER_CERT_STORE		2
#define SIGNER_CERT_SPC_CHAIN	3

typedef struct _SIGNER_CERT {
	DWORD cbSize;
	DWORD dwCertChoice;
	union {
//		LPCWSTR                pwszSpcFile;
		SIGNER_CERT_STORE_INFO *pCertStoreInfo;
//		SIGNER_SPC_CHAIN_INFO  *pSpcChainInfo;
	} ;
	HWND  hwnd;
} SIGNER_CERT, *PSIGNER_CERT;

typedef struct _SIGNER_ATTR_AUTHCODE {
	DWORD   cbSize;
	BOOL    fCommercial;
	BOOL    fIndividual;
	LPCWSTR pwszName;
	LPCWSTR pwszInfo;
} SIGNER_ATTR_AUTHCODE, *PSIGNER_ATTR_AUTHCODE;

#define SIGNER_AUTHCODE_ATTR	1
#define SIGNER_NO_ATTR			0

typedef struct _SIGNER_SIGNATURE_INFO {
	DWORD             cbSize;
	ALG_ID            algidHash;
	DWORD             dwAttrChoice;
	union {
		SIGNER_ATTR_AUTHCODE *pAttrAuthcode;
	} ;
	PCRYPT_ATTRIBUTES psAuthenticated;
	PCRYPT_ATTRIBUTES psUnauthenticated;
} SIGNER_SIGNATURE_INFO, *PSIGNER_SIGNATURE_INFO;

typedef struct _SIGNER_PROVIDER_INFO {
	DWORD   cbSize;
	LPCWSTR pwszProviderName;
	DWORD   dwProviderType;
	DWORD   dwKeySpec;
	DWORD   dwPvkChoice;
	union {
		LPWSTR pwszPvkFileName;
		LPWSTR pwszKeyContainer;
	} ;
} SIGNER_PROVIDER_INFO, *PSIGNER_PROVIDER_INFO;

typedef struct _SIGNER_CONTEXT {
	DWORD cbSize;
	DWORD cbBlob;
	BYTE  *pbBlob;
} SIGNER_CONTEXT, *PSIGNER_CONTEXT;

FUNC(SignerSignEx,				HRESULT, DWORD dwFlags, SIGNER_SUBJECT_INFO *pSubjectInfo, SIGNER_CERT *pSignerCert, SIGNER_SIGNATURE_INFO *pSignatureInfo, SIGNER_PROVIDER_INFO *pProviderInfo, LPCWSTR pwszHttpTimeStamp, PCRYPT_ATTRIBUTES psRequest, LPVOID pSipData, SIGNER_CONTEXT **ppSignerContext)
FUNC(SignerTimeStampEx,			HRESULT, DWORD dwFlags, SIGNER_SUBJECT_INFO *pSubjectInfo, LPCWSTR pwszHttpTimeStamp, PCRYPT_ATTRIBUTES psRequest, LPVOID pSipData, SIGNER_CONTEXT **ppSignerContext)
FUNC(SignerFreeSignerContext,	HRESULT, SIGNER_CONTEXT *pSignerContext)

#endif