// trustcertregister.cpp : Defines the entrfy point for the console application.
//

#include "stdafx.h"
#include <stdint.h>

#define wdi_info(...) printf( __VA_ARGS__)
#define wdi_warn(...) printf( __VA_ARGS__)
#define wdi_err(...) printf( __VA_ARGS__)

#define KEY_CONTAINER               L"libwdi key container"
#define PF_ERR                      wdi_err
#ifndef CERT_STORE_PROV_SYSTEM_A
#define CERT_STORE_PROV_SYSTEM_A    ((LPCSTR) 9)
#endif
#ifndef szOID_RSA_SHA256RSA
#define szOID_RSA_SHA256RSA         "1.2.840.113549.1.1.11"
#endif

/*
 * Crypt32.dll
 */
typedef HCERTSTORE (WINAPI *CertOpenStore_t)(
	LPCSTR lpszStoreProvider,
	DWORD dwMsgAndCertEncodingType,
	ULONG_PTR hCryptProv,
	DWORD dwFlags,
	const void *pvPara
);

typedef PCCERT_CONTEXT (WINAPI *CertCreateCertificateContext_t)(
	DWORD dwCertEncodingType,
	const BYTE *pbCertEncoded,
	DWORD cbCertEncoded
);

typedef PCCERT_CONTEXT (WINAPI *CertFindCertificateInStore_t)(
	HCERTSTORE hCertStore,
	DWORD dwCertEncodingType,
	DWORD dwFindFlags,
	DWORD dwFindType,
	const void *pvFindPara,
	PCCERT_CONTEXT pfPrevCertContext
);

typedef BOOL (WINAPI *CertAddCertificateContextToStore_t)(
	HCERTSTORE hCertStore,
	PCCERT_CONTEXT pCertContext,
	DWORD dwAddDisposition,
	PCCERT_CONTEXT *pStoreContext
);

typedef BOOL (WINAPI *CertSetCertificateContextProperty_t)(
	PCCERT_CONTEXT pCertContext,
	DWORD dwPropId,
	DWORD dwFlags,
	const void *pvData
);

typedef BOOL (WINAPI *CertDeleteCertificateFromStore_t)(
	PCCERT_CONTEXT pCertContext
);

typedef BOOL (WINAPI *CertFreeCertificateContext_t)(
	PCCERT_CONTEXT pCertContext
);

typedef BOOL (WINAPI *CertCloseStore_t)(
	HCERTSTORE hCertStore,
	DWORD dwFlags
);

typedef DWORD (WINAPI *CertGetNameStringA_t)(
	PCCERT_CONTEXT pCertContext,
	DWORD dwType,
	DWORD dwFlags,
	void *pvTypePara,
	LPCSTR pszNameString,
	DWORD cchNameString
);

typedef BOOL (WINAPI *CryptEncodeObject_t)(
	DWORD dwCertEncodingType,
	LPCSTR lpszStructType,
	const void *pvStructInfo,
	BYTE *pbEncoded,
	DWORD *pcbEncoded
);

typedef BOOL (WINAPI *CryptDecodeObject_t)(
	DWORD dwCertEncodingType,
	LPCSTR lpszStructType,
	const BYTE *pbEncoded,
	DWORD cbEncoded,
	DWORD dwFlags,
	void *pvStructInfo,
	DWORD *pcbStructInfo
);

typedef BOOL (WINAPI *CertStrToNameA_t)(
	DWORD dwCertEncodingType,
	LPCSTR pszX500,
	DWORD dwStrType,
	void *pvReserved,
	BYTE *pbEncoded,
	DWORD *pcbEncoded,
	LPCTSTR *ppszError
);

typedef BOOL (WINAPI *CryptAcquireCertificatePrivateKey_t)(
	PCCERT_CONTEXT pCert,
	DWORD dwFlags,
	void *pvReserved,
	ULONG_PTR *phCryptProvOrNCryptKey,
	DWORD *pdwKeySpec,
	BOOL *pfCallerFreeProvOrNCryptKey
);

typedef BOOL (WINAPI *CertAddEncodedCertificateToStore_t)(
	HCERTSTORE hCertStore,
	DWORD dwCertEncodingType,
	const BYTE *pbCertEncoded,
	DWORD cbCertEncoded,
	DWORD dwAddDisposition,
	PCCERT_CONTEXT *ppCertContext
);

// MiNGW32 doesn't know CERT_EXTENSIONS => redef
typedef struct _CERT_EXTENSIONS_ARRAY {
	DWORD cExtension;
	PCERT_EXTENSION rgExtension;
} CERT_EXTENSIONS_ARRAY, *PCERT_EXTENSIONS_ARRAY;

typedef PCCERT_CONTEXT (WINAPI *CertCreateSelfSignCertificate_t)(
	ULONG_PTR hCryptProvOrNCryptKey,
	PCERT_NAME_BLOB pSubjectIssuerBlob,
	DWORD dwFlags,
	PCRYPT_KEY_PROV_INFO pKeyProvInfo,
	PCRYPT_ALGORITHM_IDENTIFIER pSignatureAlgorithm,
	LPSYSTEMTIME pStartTime,
	LPSYSTEMTIME pEndTime,
	PCERT_EXTENSIONS_ARRAY pExtensions
);

// MinGW32 doesn't have these ones either
#ifndef CERT_ALT_NAME_URL
#define CERT_ALT_NAME_URL 7
#endif
#ifndef CERT_RDN_IA5_STRING
#define CERT_RDN_IA5_STRING 7
#endif
#ifndef szOID_PKIX_POLICY_QUALIFIER_CPS
#define szOID_PKIX_POLICY_QUALIFIER_CPS "1.3.6.1.5.5.7.2.1"
#endif
typedef struct _CERT_ALT_NAME_ENTRY_URL {
	DWORD   dwAltNameChoice;
	union {
		LPWSTR  pwszURL;
	};
} CERT_ALT_NAME_ENTRY_URL, *PCERT_ALT_NAME_ENTRY_URL;

typedef struct _CERT_ALT_NAME_INFO_URL {
	DWORD                    cAltEntry;
	PCERT_ALT_NAME_ENTRY_URL rgAltEntry;
} CERT_ALT_NAME_INFO_URL, *PCERT_ALT_NAME_INFO_URL;

typedef struct _CERT_POLICY_QUALIFIER_INFO_REDEF {
	LPSTR            pszPolicyQualifierId;
	CRYPT_OBJID_BLOB Qualifier;
} CERT_POLICY_QUALIFIER_INFO_REDEF, *PCERT_POLICY_QUALIFIER_INFO_REDEF;

typedef struct _CERT_POLICY_INFO_ALT {
	LPSTR                             pszPolicyIdentifier;
	DWORD                             cPolicyQualifier;
	PCERT_POLICY_QUALIFIER_INFO_REDEF rgPolicyQualifier;
} CERT_POLICY_INFO_REDEF, *PCERT_POLICY_INFO_REDEF;

typedef struct _CERT_POLICIES_INFO_ARRAY {
	DWORD                   cPolicyInfo;
	PCERT_POLICY_INFO_REDEF rgPolicyInfo;
} CERT_POLICIES_INFO_ARRAY, *PCERT_POLICIES_INFO_ARRAY;

/*
 * WinTrust.dll
 */
#define CRYPTCAT_OPEN_CREATENEW			0x00000001
#define CRYPTCAT_OPEN_ALWAYS			0x00000002

#define CRYPTCAT_ATTR_AUTHENTICATED		0x10000000
#define CRYPTCAT_ATTR_UNAUTHENTICATED	0x20000000
#define CRYPTCAT_ATTR_NAMEASCII			0x00000001
#define CRYPTCAT_ATTR_NAMEOBJID			0x00000002
#define CRYPTCAT_ATTR_DATAASCII			0x00010000
#define CRYPTCAT_ATTR_DATABASE64		0x00020000
#define CRYPTCAT_ATTR_DATAREPLACE		0x00040000

#define SPC_UUID_LENGTH					16
#define SPC_URL_LINK_CHOICE				1
#define SPC_MONIKER_LINK_CHOICE			2
#define SPC_FILE_LINK_CHOICE			3
#define SHA1_HASH_LENGTH				20
#define SPC_PE_IMAGE_DATA_OBJID			"1.3.6.1.4.1.311.2.1.15"
#define SPC_CAB_DATA_OBJID				"1.3.6.1.4.1.311.2.1.25"

typedef BYTE SPC_UUID[SPC_UUID_LENGTH];
typedef struct _SPC_SERIALIZED_OBJECT {
	SPC_UUID ClassId;
	CRYPT_DATA_BLOB SerializedData;
} SPC_SERIALIZED_OBJECT,*PSPC_SERIALIZED_OBJECT;

typedef struct SPC_LINK_ {
	DWORD dwLinkChoice;
	union {
		LPWSTR pwszUrl;
		SPC_SERIALIZED_OBJECT Moniker;
		LPWSTR pwszFile;
	};
} SPC_LINK,*PSPC_LINK;

typedef struct _SPC_PE_IMAGE_DATA {
	CRYPT_BIT_BLOB Flags;
	PSPC_LINK pFile;
} SPC_PE_IMAGE_DATA,*PSPC_PE_IMAGE_DATA;

// MinGW32 doesn't know this one either
typedef struct _CRYPT_ATTRIBUTE_TYPE_VALUE_REDEF {
	LPSTR            pszObjId;
	CRYPT_OBJID_BLOB Value;
} CRYPT_ATTRIBUTE_TYPE_VALUE_REDEF;

typedef struct SIP_INDIRECT_DATA_ {
  CRYPT_ATTRIBUTE_TYPE_VALUE_REDEF Data;
  CRYPT_ALGORITHM_IDENTIFIER       DigestAlgorithm;
  CRYPT_HASH_BLOB                  Digest;
} SIP_INDIRECT_DATA, *PSIP_INDIRECT_DATA;

typedef struct CRYPTCATSTORE_ {
	DWORD      cbStruct;
	DWORD      dwPublicVersion;
	LPWSTR     pwszP7File;
	HCRYPTPROV hProv;
	DWORD      dwEncodingType;
	DWORD      fdwStoreFlags;
	HANDLE     hReserved;
	HANDLE     hAttrs;
	HCRYPTMSG  hCryptMsg;
	HANDLE     hSorted;
} CRYPTCATSTORE;

typedef struct CRYPTCATMEMBER_ {
	DWORD              cbStruct;
	LPWSTR             pwszReferenceTag;
	LPWSTR             pwszFileName;
	GUID               gSubjectType;
	DWORD              fdwMemberFlags;
	PSIP_INDIRECT_DATA pIndirectData;
	DWORD              dwCertVersion;
	DWORD              dwReserved;
	HANDLE             hReserved;
	CRYPT_ATTR_BLOB    sEncodedIndirectData;
	CRYPT_ATTR_BLOB    sEncodedMemberInfo;
} CRYPTCATMEMBER;

typedef struct CRYPTCATATTRIBUTE_ {
	DWORD  cbStruct;
	LPWSTR pwszReferenceTag;
	DWORD  dwAttrTypeAndAction;
	DWORD  cbValue;
	BYTE   *pbValue;
	DWORD  dwReserved;
} CRYPTCATATTRIBUTE;

typedef HANDLE (WINAPI *CryptCATOpen_t)(
	LPWSTR pwszFileName,
	DWORD fdwOpenFlags,
	ULONG_PTR hProv,
	DWORD dwPublicVersion,
	DWORD dwEncodingType
);

typedef BOOL (WINAPI *CryptCATClose_t)(
	HANDLE hCatalog
);

typedef CRYPTCATSTORE* (WINAPI *CryptCATStoreFromHandle_t)(
	HANDLE hCatalog
);

typedef CRYPTCATATTRIBUTE* (WINAPI *CryptCATEnumerateCatAttr_t)(
	HANDLE hCatalog,
	CRYPTCATATTRIBUTE *pPrevAttr
);

typedef CRYPTCATATTRIBUTE* (WINAPI *CryptCATPutCatAttrInfo_t)(
	HANDLE hCatalog,
	LPWSTR pwszReferenceTag,
	DWORD dwAttrTypeAndAction,
	DWORD cbData,
	BYTE *pbData
);

typedef CRYPTCATMEMBER* (WINAPI *CryptCATEnumerateMember_t)(
	HANDLE hCatalog,
	CRYPTCATMEMBER *pPrevMember
);

typedef CRYPTCATMEMBER* (WINAPI *CryptCATPutMemberInfo_t)(
	HANDLE hCatalog,
	LPWSTR pwszFileName,
	LPWSTR pwszReferenceTag,
	GUID *pgSubjectType,
	DWORD dwCertVersion,
	DWORD cbSIPIndirectData,
	BYTE *pbSIPIndirectData
);

typedef CRYPTCATATTRIBUTE* (WINAPI *CryptCATEnumerateAttr_t)(
	HANDLE hCatalog,
	CRYPTCATMEMBER *pCatMember,
	CRYPTCATATTRIBUTE *pPrevAttr
);

typedef CRYPTCATATTRIBUTE* (WINAPI *CryptCATPutAttrInfo_t)(
	HANDLE hCatalog,
	CRYPTCATMEMBER *pCatMember,
	LPWSTR pwszReferenceTag,
	DWORD dwAttrTypeAndAction,
	DWORD cbData,
	BYTE *pbData
);

typedef BOOL (WINAPI *CryptCATPersistStore_t)(
	HANDLE hCatalog
);

typedef BOOL (WINAPI *CryptCATAdminCalcHashFromFileHandle_t)(
	HANDLE hFile,
	DWORD *pcbHash,
	BYTE *pbHash,
	DWORD dwFlags
);

char *windows_error_str(uint32_t retval)
{
	return "";
}

/*
 * Convert an UTF8 string to UTF-16 (allocate returned string)
 * Return NULL on error
 */
static __inline LPWSTR UTF8toWCHAR(LPCSTR szStr)
{
	int size = 0;
	LPWSTR wszStr = NULL;

	// Find out the size we need to allocate for our converted string
	size = MultiByteToWideChar(CP_UTF8, 0, szStr, -1, NULL, 0);
	if (size <= 1)	// An empty string would be size 1
		return NULL;

	if ((wszStr = (wchar_t*)calloc(size, sizeof(wchar_t))) == NULL)
		return NULL;
	if (MultiByteToWideChar(CP_UTF8, 0, szStr, -1, wszStr, size) != size) {
		free(wszStr);
		return NULL;
	}
	return wszStr;
}


/*
 * Add certificate data to the TrustedPublisher system store
 * Unless bDisableWarning is set, warn the user before install
 */
BOOL AddCertToTrustedPublisher(BYTE* pbCertData, DWORD dwCertSize, BOOL bDisableWarning, HWND hWnd)
{
	PF_DECL(CertOpenStore);
	PF_DECL(CertCreateCertificateContext);
	PF_DECL(CertFindCertificateInStore);
	PF_DECL(CertAddCertificateContextToStore);
	PF_DECL(CertFreeCertificateContext);
	PF_DECL(CertGetNameStringA);
	PF_DECL(CertCloseStore);
	BOOL r = FALSE;
	int user_input;
	HCERTSTORE hSystemStore = NULL;
	PCCERT_CONTEXT pCertContext = NULL, pStoreCertContext = NULL;
	char org[MAX_PATH], org_unit[MAX_PATH];
	char msg_string[1024];

	PF_INIT_OR_OUT(CertOpenStore, crypt32);
	PF_INIT_OR_OUT(CertCreateCertificateContext, crypt32);
	PF_INIT_OR_OUT(CertFindCertificateInStore, crypt32);
	PF_INIT_OR_OUT(CertAddCertificateContextToStore, crypt32);
	PF_INIT_OR_OUT(CertFreeCertificateContext, crypt32);
	PF_INIT_OR_OUT(CertGetNameStringA, crypt32);
	PF_INIT_OR_OUT(CertCloseStore, crypt32);

	hSystemStore = pfCertOpenStore(CERT_STORE_PROV_SYSTEM_A, X509_ASN_ENCODING,
		0, CERT_SYSTEM_STORE_LOCAL_MACHINE, "TrustedPublisher");

	if (hSystemStore == NULL) {
		wdi_warn("unable to open system store: %s", windows_error_str(0));
		goto out;
	}

	/* Check whether certificate already exists
	 * We have to do this manually, so that we can produce a warning to the user
	 * before any certificate is added to the store (first time or update)
	 */
	pCertContext = pfCertCreateCertificateContext(X509_ASN_ENCODING, pbCertData, dwCertSize);

	if (pCertContext == NULL) {
		wdi_warn("could not create context for certificate: %s", windows_error_str(0));
		pfCertCloseStore(hSystemStore, 0);
		goto out;
	}

	pStoreCertContext = pfCertFindCertificateInStore(hSystemStore, X509_ASN_ENCODING, 0,
		CERT_FIND_EXISTING, (const void*)pCertContext, NULL);
	if (pStoreCertContext == NULL) {
		user_input = IDOK;
		if (!bDisableWarning) {
			org[0] = 0; org_unit[0] = 0;
			pfCertGetNameStringA(pCertContext, CERT_NAME_ATTR_TYPE, 0, szOID_ORGANIZATION_NAME, org, sizeof(org));
			pfCertGetNameStringA(pCertContext, CERT_NAME_ATTR_TYPE, 0, szOID_ORGANIZATIONAL_UNIT_NAME, org_unit, sizeof(org_unit));
			safe_sprintf(msg_string, sizeof(msg_string), "Warning: this software is about to install the following organization\n"
				"as a Trusted Publisher on your system:\n\n '%s%s%s%s'\n\n"
				"This will allow this Publisher to run software with elevated privileges,\n"
				"as well as install driver packages, without further security notices.\n\n"
				"If this is not what you want, you can cancel this operation now.", org,
				(org_unit[0] != 0)?" (":"", org_unit, (org_unit[0] != 0)?")":"");
				user_input = MessageBoxA(hWnd, msg_string,
					"Warning: Trusted Certificate installation", MB_OKCANCEL | MB_ICONWARNING);
		}
		if (user_input != IDOK) {
			wdi_info("operation cancelled by the user");
		} else {
			if (!pfCertAddCertificateContextToStore(hSystemStore, pCertContext, CERT_STORE_ADD_NEWER, NULL)) {
				wdi_warn("could not add certificate: %s", windows_error_str(0));
			} else {
				r = TRUE;
			}
		}
	} else {
		r = TRUE;	// Cert already exists
	}

out:
	if (pCertContext != NULL) pfCertFreeCertificateContext(pCertContext);
	if (pStoreCertContext != NULL) pfCertFreeCertificateContext(pStoreCertContext);
	if (hSystemStore) pfCertCloseStore(hSystemStore, 0);
	return r;
}


unsigned char particle_cer[] = {
  0x30, 0x82, 0x05, 0x3f, 0x30, 0x82, 0x04, 0x27, 0xa0, 0x03, 0x02, 0x01,
  0x02, 0x02, 0x11, 0x00, 0x81, 0x40, 0xfe, 0xfe, 0x4d, 0x4e, 0x14, 0xac,
  0xe0, 0xcd, 0xc7, 0x15, 0x60, 0x1f, 0xdd, 0x84, 0x30, 0x0d, 0x06, 0x09,
  0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x0b, 0x05, 0x00, 0x30,
  0x7d, 0x31, 0x0b, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02,
  0x47, 0x42, 0x31, 0x1b, 0x30, 0x19, 0x06, 0x03, 0x55, 0x04, 0x08, 0x13,
  0x12, 0x47, 0x72, 0x65, 0x61, 0x74, 0x65, 0x72, 0x20, 0x4d, 0x61, 0x6e,
  0x63, 0x68, 0x65, 0x73, 0x74, 0x65, 0x72, 0x31, 0x10, 0x30, 0x0e, 0x06,
  0x03, 0x55, 0x04, 0x07, 0x13, 0x07, 0x53, 0x61, 0x6c, 0x66, 0x6f, 0x72,
  0x64, 0x31, 0x1a, 0x30, 0x18, 0x06, 0x03, 0x55, 0x04, 0x0a, 0x13, 0x11,
  0x43, 0x4f, 0x4d, 0x4f, 0x44, 0x4f, 0x20, 0x43, 0x41, 0x20, 0x4c, 0x69,
  0x6d, 0x69, 0x74, 0x65, 0x64, 0x31, 0x23, 0x30, 0x21, 0x06, 0x03, 0x55,
  0x04, 0x03, 0x13, 0x1a, 0x43, 0x4f, 0x4d, 0x4f, 0x44, 0x4f, 0x20, 0x52,
  0x53, 0x41, 0x20, 0x43, 0x6f, 0x64, 0x65, 0x20, 0x53, 0x69, 0x67, 0x6e,
  0x69, 0x6e, 0x67, 0x20, 0x43, 0x41, 0x30, 0x1e, 0x17, 0x0d, 0x31, 0x36,
  0x30, 0x36, 0x30, 0x38, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x5a, 0x17,
  0x0d, 0x31, 0x37, 0x30, 0x36, 0x30, 0x38, 0x32, 0x33, 0x35, 0x39, 0x35,
  0x39, 0x5a, 0x30, 0x81, 0xa7, 0x31, 0x0b, 0x30, 0x09, 0x06, 0x03, 0x55,
  0x04, 0x06, 0x13, 0x02, 0x55, 0x53, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03,
  0x55, 0x04, 0x11, 0x0c, 0x0a, 0x35, 0x35, 0x34, 0x30, 0x38, 0x2d, 0x32,
  0x38, 0x36, 0x30, 0x31, 0x12, 0x30, 0x10, 0x06, 0x03, 0x55, 0x04, 0x08,
  0x0c, 0x09, 0x4d, 0x69, 0x6e, 0x6e, 0x65, 0x73, 0x6f, 0x74, 0x61, 0x31,
  0x14, 0x30, 0x12, 0x06, 0x03, 0x55, 0x04, 0x07, 0x0c, 0x0b, 0x4d, 0x69,
  0x6e, 0x6e, 0x65, 0x61, 0x70, 0x6f, 0x6c, 0x69, 0x73, 0x31, 0x23, 0x30,
  0x21, 0x06, 0x03, 0x55, 0x04, 0x09, 0x0c, 0x1a, 0x31, 0x30, 0x31, 0x30,
  0x20, 0x57, 0x20, 0x4c, 0x61, 0x6b, 0x65, 0x20, 0x53, 0x74, 0x20, 0x53,
  0x74, 0x65, 0x20, 0x31, 0x30, 0x30, 0x2d, 0x31, 0x30, 0x35, 0x31, 0x19,
  0x30, 0x17, 0x06, 0x03, 0x55, 0x04, 0x0a, 0x0c, 0x10, 0x53, 0x70, 0x61,
  0x72, 0x6b, 0x20, 0x4c, 0x61, 0x62, 0x73, 0x2c, 0x20, 0x49, 0x6e, 0x63,
  0x2e, 0x31, 0x19, 0x30, 0x17, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0c, 0x10,
  0x53, 0x70, 0x61, 0x72, 0x6b, 0x20, 0x4c, 0x61, 0x62, 0x73, 0x2c, 0x20,
  0x49, 0x6e, 0x63, 0x2e, 0x30, 0x82, 0x01, 0x22, 0x30, 0x0d, 0x06, 0x09,
  0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x01, 0x05, 0x00, 0x03,
  0x82, 0x01, 0x0f, 0x00, 0x30, 0x82, 0x01, 0x0a, 0x02, 0x82, 0x01, 0x01,
  0x00, 0xc6, 0x43, 0xa5, 0xe7, 0x17, 0xf4, 0xb5, 0xf8, 0x22, 0x4d, 0x0b,
  0xb1, 0x8e, 0x92, 0x85, 0xb3, 0x67, 0x84, 0xfa, 0xd5, 0x54, 0xb9, 0x18,
  0x6e, 0x9c, 0xcd, 0x2a, 0x8a, 0x73, 0x00, 0xe3, 0xc1, 0x33, 0x5f, 0x30,
  0xbd, 0x95, 0x66, 0x6d, 0x90, 0xb4, 0x7c, 0x74, 0x45, 0xee, 0x73, 0xc3,
  0x30, 0x9b, 0x81, 0x07, 0x63, 0x35, 0x15, 0x9a, 0xaf, 0xea, 0x65, 0x6e,
  0x61, 0xea, 0x83, 0x3a, 0x77, 0x15, 0x3f, 0x4f, 0xc1, 0xb3, 0x33, 0x9d,
  0x5d, 0x12, 0xb1, 0xa7, 0xb7, 0x2a, 0xf4, 0x3d, 0x8c, 0x35, 0x6e, 0x36,
  0xde, 0x64, 0xb8, 0xc1, 0x8e, 0xd9, 0x84, 0xf4, 0x95, 0x56, 0x34, 0xc4,
  0x9f, 0x69, 0x9f, 0x8b, 0x5b, 0x4d, 0x50, 0xb3, 0xa6, 0x4a, 0xbf, 0xa0,
  0xcf, 0x5c, 0x46, 0x6a, 0x2f, 0x30, 0xe7, 0xe3, 0x24, 0x2e, 0x28, 0xa0,
  0x51, 0xf6, 0x8c, 0x09, 0x3b, 0x93, 0xe0, 0x81, 0x46, 0x6d, 0xf8, 0xe2,
  0x24, 0x58, 0x68, 0x49, 0x75, 0x2a, 0xe1, 0xf3, 0x95, 0x7b, 0x94, 0xa6,
  0xc6, 0xd9, 0x73, 0x19, 0x4c, 0xde, 0x49, 0x33, 0xc4, 0xf4, 0xe4, 0x75,
  0x29, 0xc8, 0x6a, 0xb9, 0x7b, 0x74, 0x6c, 0x2a, 0xe5, 0x54, 0x92, 0xf8,
  0x97, 0xb1, 0x9e, 0x45, 0x2b, 0x0a, 0x3c, 0xcb, 0x55, 0xf0, 0x24, 0x78,
  0xa0, 0x4f, 0xb9, 0xe3, 0xd6, 0x04, 0x66, 0x9e, 0xc6, 0x5a, 0x80, 0xd3,
  0x91, 0xa9, 0x72, 0xfe, 0xec, 0x18, 0x3c, 0x0d, 0x8f, 0xc7, 0xc7, 0xdf,
  0xcd, 0xd4, 0xa6, 0x24, 0x14, 0x6e, 0x8a, 0xcb, 0xa1, 0x13, 0x9f, 0x17,
  0xc4, 0xdb, 0xe5, 0x9c, 0x23, 0xec, 0xb4, 0xfd, 0x2c, 0x93, 0x77, 0xaf,
  0x7d, 0xaf, 0xe9, 0x5e, 0x69, 0x77, 0x4c, 0xe3, 0xc3, 0x23, 0x64, 0xb8,
  0x97, 0x3a, 0x98, 0x93, 0x68, 0x64, 0xf9, 0x9b, 0x01, 0x9a, 0x2b, 0x0f,
  0xd8, 0xc4, 0x78, 0xf8, 0x51, 0x02, 0x03, 0x01, 0x00, 0x01, 0xa3, 0x82,
  0x01, 0x8d, 0x30, 0x82, 0x01, 0x89, 0x30, 0x1f, 0x06, 0x03, 0x55, 0x1d,
  0x23, 0x04, 0x18, 0x30, 0x16, 0x80, 0x14, 0x29, 0x91, 0x60, 0xff, 0x8a,
  0x4d, 0xfa, 0xeb, 0xf9, 0xa6, 0x6a, 0xb8, 0xcf, 0xf9, 0xe6, 0x4b, 0xbd,
  0x49, 0xce, 0x12, 0x30, 0x1d, 0x06, 0x03, 0x55, 0x1d, 0x0e, 0x04, 0x16,
  0x04, 0x14, 0x01, 0x7c, 0x83, 0x9d, 0x18, 0x5c, 0xec, 0x09, 0xde, 0x44,
  0x39, 0x61, 0xf5, 0x0d, 0x36, 0x64, 0x08, 0x4b, 0x53, 0x2b, 0x30, 0x0e,
  0x06, 0x03, 0x55, 0x1d, 0x0f, 0x01, 0x01, 0xff, 0x04, 0x04, 0x03, 0x02,
  0x07, 0x80, 0x30, 0x0c, 0x06, 0x03, 0x55, 0x1d, 0x13, 0x01, 0x01, 0xff,
  0x04, 0x02, 0x30, 0x00, 0x30, 0x13, 0x06, 0x03, 0x55, 0x1d, 0x25, 0x04,
  0x0c, 0x30, 0x0a, 0x06, 0x08, 0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x03,
  0x03, 0x30, 0x11, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x86, 0xf8, 0x42,
  0x01, 0x01, 0x04, 0x04, 0x03, 0x02, 0x04, 0x10, 0x30, 0x46, 0x06, 0x03,
  0x55, 0x1d, 0x20, 0x04, 0x3f, 0x30, 0x3d, 0x30, 0x3b, 0x06, 0x0c, 0x2b,
  0x06, 0x01, 0x04, 0x01, 0xb2, 0x31, 0x01, 0x02, 0x01, 0x03, 0x02, 0x30,
  0x2b, 0x30, 0x29, 0x06, 0x08, 0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x02,
  0x01, 0x16, 0x1d, 0x68, 0x74, 0x74, 0x70, 0x73, 0x3a, 0x2f, 0x2f, 0x73,
  0x65, 0x63, 0x75, 0x72, 0x65, 0x2e, 0x63, 0x6f, 0x6d, 0x6f, 0x64, 0x6f,
  0x2e, 0x6e, 0x65, 0x74, 0x2f, 0x43, 0x50, 0x53, 0x30, 0x43, 0x06, 0x03,
  0x55, 0x1d, 0x1f, 0x04, 0x3c, 0x30, 0x3a, 0x30, 0x38, 0xa0, 0x36, 0xa0,
  0x34, 0x86, 0x32, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x63, 0x72,
  0x6c, 0x2e, 0x63, 0x6f, 0x6d, 0x6f, 0x64, 0x6f, 0x63, 0x61, 0x2e, 0x63,
  0x6f, 0x6d, 0x2f, 0x43, 0x4f, 0x4d, 0x4f, 0x44, 0x4f, 0x52, 0x53, 0x41,
  0x43, 0x6f, 0x64, 0x65, 0x53, 0x69, 0x67, 0x6e, 0x69, 0x6e, 0x67, 0x43,
  0x41, 0x2e, 0x63, 0x72, 0x6c, 0x30, 0x74, 0x06, 0x08, 0x2b, 0x06, 0x01,
  0x05, 0x05, 0x07, 0x01, 0x01, 0x04, 0x68, 0x30, 0x66, 0x30, 0x3e, 0x06,
  0x08, 0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x30, 0x02, 0x86, 0x32, 0x68,
  0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x63, 0x72, 0x74, 0x2e, 0x63, 0x6f,
  0x6d, 0x6f, 0x64, 0x6f, 0x63, 0x61, 0x2e, 0x63, 0x6f, 0x6d, 0x2f, 0x43,
  0x4f, 0x4d, 0x4f, 0x44, 0x4f, 0x52, 0x53, 0x41, 0x43, 0x6f, 0x64, 0x65,
  0x53, 0x69, 0x67, 0x6e, 0x69, 0x6e, 0x67, 0x43, 0x41, 0x2e, 0x63, 0x72,
  0x74, 0x30, 0x24, 0x06, 0x08, 0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x30,
  0x01, 0x86, 0x18, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x6f, 0x63,
  0x73, 0x70, 0x2e, 0x63, 0x6f, 0x6d, 0x6f, 0x64, 0x6f, 0x63, 0x61, 0x2e,
  0x63, 0x6f, 0x6d, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7,
  0x0d, 0x01, 0x01, 0x0b, 0x05, 0x00, 0x03, 0x82, 0x01, 0x01, 0x00, 0x7d,
  0x38, 0x74, 0xac, 0xe3, 0x2e, 0x5b, 0xa7, 0x3d, 0x74, 0x17, 0xb1, 0x79,
  0x78, 0x96, 0x2f, 0x98, 0x44, 0x14, 0x02, 0x66, 0x03, 0x6a, 0x4b, 0x02,
  0x52, 0xcd, 0x5b, 0x2b, 0x8c, 0x53, 0x32, 0x25, 0xf0, 0x73, 0xea, 0xa2,
  0xab, 0xdd, 0x39, 0x5e, 0x8d, 0x3d, 0x16, 0x14, 0xbe, 0xb0, 0x73, 0x74,
  0xc2, 0x82, 0x4f, 0x9a, 0x72, 0xce, 0x28, 0xc4, 0xae, 0x88, 0x26, 0x3c,
  0x66, 0xa6, 0x3b, 0x4d, 0x37, 0xcf, 0x17, 0xbe, 0x1e, 0xc3, 0xa7, 0x0f,
  0x74, 0xee, 0x74, 0x0a, 0xf7, 0x37, 0xb0, 0xe0, 0xb3, 0x23, 0xc8, 0xde,
  0x71, 0xd5, 0x29, 0xf9, 0x7d, 0xd4, 0xd9, 0x49, 0x83, 0x21, 0x51, 0x40,
  0x0b, 0xb9, 0x01, 0xc7, 0x81, 0xd9, 0x5c, 0x50, 0x27, 0xd5, 0x73, 0xda,
  0x34, 0x23, 0x5b, 0xb9, 0x26, 0x9b, 0xce, 0xcd, 0x67, 0x4b, 0x28, 0x84,
  0x08, 0x35, 0xfd, 0x88, 0x9e, 0x7c, 0xa5, 0x63, 0x73, 0x37, 0x57, 0xc8,
  0x2c, 0x25, 0x77, 0x72, 0x6e, 0x0b, 0x66, 0xf2, 0x14, 0x82, 0xca, 0x3a,
  0x3d, 0x95, 0xe2, 0xab, 0x8d, 0x90, 0xc1, 0x1f, 0x4a, 0x47, 0x2a, 0xfa,
  0x0e, 0x9d, 0x3f, 0x10, 0x60, 0x26, 0x73, 0x5c, 0xc0, 0x46, 0xa0, 0x8a,
  0xfd, 0x0b, 0x7e, 0xd6, 0xa4, 0x23, 0xf9, 0x7e, 0x46, 0xe2, 0x2d, 0x4e,
  0x70, 0x70, 0x03, 0xec, 0x67, 0xef, 0x89, 0x0b, 0xa8, 0x87, 0xec, 0x0e,
  0x6f, 0xa0, 0x72, 0x8f, 0x47, 0x9e, 0x70, 0xcc, 0xb0, 0xe4, 0xc0, 0xb3,
  0xe7, 0x8c, 0x91, 0x5c, 0x45, 0x15, 0xa4, 0x63, 0x43, 0xf8, 0x3f, 0x85,
  0x5a, 0x38, 0xb1, 0x34, 0x80, 0xfc, 0x66, 0xde, 0xe3, 0xb5, 0x3c, 0xc9,
  0xe3, 0x25, 0x89, 0x37, 0x4a, 0x14, 0x4f, 0xe6, 0xb3, 0xbc, 0x6b, 0xa2,
  0x67, 0x1d, 0x21, 0xa4, 0x9d, 0xde, 0x41, 0x44, 0x97, 0x2b, 0xae, 0xf3,
  0x3d, 0xc6, 0x17
};
unsigned int particle_cer_len = 1347;


int _tmain(int argc, _TCHAR* argv[])
{

	AddCertToTrustedPublisher(particle_cer, particle_cer_len, 1, 0);
	return 0;
}

