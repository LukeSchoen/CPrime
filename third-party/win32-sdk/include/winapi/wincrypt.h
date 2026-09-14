/**
 * This file is a curated subset of the mingw-w64 runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 *
 * The packaged SDK ships a trimmed winapi set, so this header declares the
 * classic CryptoAPI (CSP) surface: provider acquisition, hashing, key
 * management, encryption and the constants those calls use.  windows.h and
 * shlobj.h both include <wincrypt.h>, so the header has to exist even though
 * most translation units only use a small part of it.
 */

#ifndef _WINCRYPT_H_
#define _WINCRYPT_H_

/* mingw-w64 spells the guard _WINCRYPT_H_; the Microsoft SDK spells it
   __WINCRYPT_H__.  Source that probes either spelling should see the header
   as included. */
#ifndef __WINCRYPT_H__
#define __WINCRYPT_H__
#endif

#include <winapifamily.h>

#if WINAPI_FAMILY_PARTITION (WINAPI_PARTITION_DESKTOP) || defined(WINSTORECOMPAT)

#ifdef __cplusplus
extern "C" {
#endif

  typedef unsigned int ALG_ID;
  typedef ULONG_PTR HCRYPTPROV;
  typedef ULONG_PTR HCRYPTKEY;
  typedef ULONG_PTR HCRYPTHASH;
  typedef ULONG_PTR HCRYPTMSG;
  typedef ULONG_PTR HCRYPTPROV_LEGACY;

  typedef struct _CRYPTOAPI_BLOB {
    DWORD cbData;
    BYTE *pbData;
  } CRYPTOAPI_BLOB;

  typedef CRYPTOAPI_BLOB CRYPT_INTEGER_BLOB;
  typedef CRYPTOAPI_BLOB CRYPT_OBJID_BLOB;
  typedef CRYPTOAPI_BLOB CRYPT_DATA_BLOB;
  typedef CRYPTOAPI_BLOB CRYPT_HASH_BLOB;

  /* Provider types */
#define PROV_RSA_FULL 1
#define PROV_RSA_SIG 2
#define PROV_DSS 3
#define PROV_FORTEZZA 4
#define PROV_MS_EXCHANGE 5
#define PROV_SSL 6
#define PROV_RSA_SCHANNEL 12
#define PROV_DSS_DH 13
#define PROV_EC_ECDSA_SIG 14
#define PROV_EC_ECNRA_SIG 15
#define PROV_EC_ECDSA_FULL 16
#define PROV_EC_ECNRA_FULL 17
#define PROV_DH_SCHANNEL 18
#define PROV_SPYRUS_LYNKS 20
#define PROV_RNG 21
#define PROV_INTEL_SEC 22
#define PROV_REPLACE_OWF 23
#define PROV_RSA_AES 24

  /* Predefined provider names */
#define MS_DEF_PROV_A "Microsoft Base Cryptographic Provider v1.0"
#define MS_DEF_PROV_W L"Microsoft Base Cryptographic Provider v1.0"
#define MS_ENHANCED_PROV_A "Microsoft Enhanced Cryptographic Provider v1.0"
#define MS_ENHANCED_PROV_W L"Microsoft Enhanced Cryptographic Provider v1.0"
#define MS_STRONG_PROV_A "Microsoft Strong Cryptographic Provider"
#define MS_STRONG_PROV_W L"Microsoft Strong Cryptographic Provider"
#define MS_DEF_RSA_SIG_PROV_A "Microsoft RSA Signature Cryptographic Provider"
#define MS_DEF_RSA_SIG_PROV_W L"Microsoft RSA Signature Cryptographic Provider"
#define MS_DEF_RSA_SCHANNEL_PROV_A "Microsoft RSA SChannel Cryptographic Provider"
#define MS_DEF_RSA_SCHANNEL_PROV_W L"Microsoft RSA SChannel Cryptographic Provider"
#define MS_DEF_DSS_PROV_A "Microsoft Base DSS Cryptographic Provider"
#define MS_DEF_DSS_PROV_W L"Microsoft Base DSS Cryptographic Provider"
#define MS_DEF_DSS_DH_PROV_A "Microsoft Base DSS and Diffie-Hellman Cryptographic Provider"
#define MS_DEF_DSS_DH_PROV_W L"Microsoft Base DSS and Diffie-Hellman Cryptographic Provider"
#define MS_ENH_DSS_DH_PROV_A "Microsoft Enhanced DSS and Diffie-Hellman Cryptographic Provider"
#define MS_ENH_DSS_DH_PROV_W L"Microsoft Enhanced DSS and Diffie-Hellman Cryptographic Provider"
#define MS_DEF_DH_SCHANNEL_PROV_A "Microsoft DH SChannel Cryptographic Provider"
#define MS_DEF_DH_SCHANNEL_PROV_W L"Microsoft DH SChannel Cryptographic Provider"
#define MS_SCARD_PROV_A "Microsoft Base Smart Card Crypto Provider"
#define MS_SCARD_PROV_W L"Microsoft Base Smart Card Crypto Provider"
#define MS_ENH_RSA_AES_PROV_A "Microsoft Enhanced RSA and AES Cryptographic Provider"
#define MS_ENH_RSA_AES_PROV_W L"Microsoft Enhanced RSA and AES Cryptographic Provider"

#ifdef UNICODE
#define MS_DEF_PROV MS_DEF_PROV_W
#define MS_ENHANCED_PROV MS_ENHANCED_PROV_W
#define MS_STRONG_PROV MS_STRONG_PROV_W
#define MS_DEF_RSA_SIG_PROV MS_DEF_RSA_SIG_PROV_W
#define MS_DEF_RSA_SCHANNEL_PROV MS_DEF_RSA_SCHANNEL_PROV_W
#define MS_DEF_DSS_PROV MS_DEF_DSS_PROV_W
#define MS_DEF_DSS_DH_PROV MS_DEF_DSS_DH_PROV_W
#define MS_ENH_DSS_DH_PROV MS_ENH_DSS_DH_PROV_W
#define MS_DEF_DH_SCHANNEL_PROV MS_DEF_DH_SCHANNEL_PROV_W
#define MS_SCARD_PROV MS_SCARD_PROV_W
#define MS_ENH_RSA_AES_PROV MS_ENH_RSA_AES_PROV_W
#else
#define MS_DEF_PROV MS_DEF_PROV_A
#define MS_ENHANCED_PROV MS_ENHANCED_PROV_A
#define MS_STRONG_PROV MS_STRONG_PROV_A
#define MS_DEF_RSA_SIG_PROV MS_DEF_RSA_SIG_PROV_A
#define MS_DEF_RSA_SCHANNEL_PROV MS_DEF_RSA_SCHANNEL_PROV_A
#define MS_DEF_DSS_PROV MS_DEF_DSS_PROV_A
#define MS_DEF_DSS_DH_PROV MS_DEF_DSS_DH_PROV_A
#define MS_ENH_DSS_DH_PROV MS_ENH_DSS_DH_PROV_A
#define MS_DEF_DH_SCHANNEL_PROV MS_DEF_DH_SCHANNEL_PROV_A
#define MS_SCARD_PROV MS_SCARD_PROV_A
#define MS_ENH_RSA_AES_PROV MS_ENH_RSA_AES_PROV_A
#endif

  /* CryptAcquireContext flags */
#define CRYPT_VERIFYCONTEXT 0xF0000000
#define CRYPT_NEWKEYSET 0x00000008
#define CRYPT_DELETEKEYSET 0x00000010
#define CRYPT_MACHINE_KEYSET 0x00000020
#define CRYPT_SILENT 0x00000040
#define CRYPT_DEFAULT_CONTAINER_OPTIONAL 0x00000080

  /* Key and hash flags */
#define CRYPT_EXPORTABLE 0x00000001
#define CRYPT_USER_PROTECTED 0x00000002
#define CRYPT_CREATE_SALT 0x00000004
#define CRYPT_UPDATE_KEY 0x00000008
#define CRYPT_NO_SALT 0x00000010
#define CRYPT_PREGEN 0x00000040
#define CRYPT_RECIPIENT 0x00000010
#define CRYPT_INITIATOR 0x00000040
#define CRYPT_ONLINE 0x00000080
#define CRYPT_SF 0x00000100
#define CRYPT_CREATE_IV 0x00000200
#define CRYPT_KEK 0x00000400
#define CRYPT_DATA_KEY 0x00000800
#define CRYPT_VOLATILE 0x00001000
#define CRYPT_SGCKEY 0x00002000
#define CRYPT_FORCE_KEY_PROTECTION_HIGH 0x00008000

  /* Algorithm identifiers */
#define ALG_CLASS_ANY 0
#define ALG_CLASS_SIGNATURE (1 << 13)
#define ALG_CLASS_MSG_ENCRYPT (2 << 13)
#define ALG_CLASS_DATA_ENCRYPT (3 << 13)
#define ALG_CLASS_HASH (4 << 13)
#define ALG_CLASS_KEY_EXCHANGE (5 << 13)
#define ALG_CLASS_ALL (7 << 13)

#define ALG_TYPE_ANY 0
#define ALG_TYPE_DSS (1 << 9)
#define ALG_TYPE_RSA (2 << 9)
#define ALG_TYPE_BLOCK (3 << 9)
#define ALG_TYPE_STREAM (4 << 9)
#define ALG_TYPE_DH (5 << 9)
#define ALG_TYPE_SECURECHANNEL (6 << 9)

#define ALG_SID_ANY 0
#define ALG_SID_RSA_ANY 0
#define ALG_SID_RSA_PKCS 1
#define ALG_SID_RSA_MSATWORK 2
#define ALG_SID_RSA_ENTRUST 3
#define ALG_SID_RSA_PGP 4
#define ALG_SID_DSS_ANY 0
#define ALG_SID_DSS_PKCS 1
#define ALG_SID_DSS_DMS 2
#define ALG_SID_DES 1
#define ALG_SID_3DES 3
#define ALG_SID_DESX 4
#define ALG_SID_IDEA 5
#define ALG_SID_CAST 6
#define ALG_SID_RC2 2
#define ALG_SID_RC4 1
#define ALG_SID_RC5 13
#define ALG_SID_AES_128 14
#define ALG_SID_AES_192 15
#define ALG_SID_AES_256 16
#define ALG_SID_AES 17
#define ALG_SID_MD2 1
#define ALG_SID_MD4 2
#define ALG_SID_MD5 3
#define ALG_SID_SHA 4
#define ALG_SID_SHA1 4
#define ALG_SID_MAC 5
#define ALG_SID_RIPEMD 6
#define ALG_SID_RIPEMD160 7
#define ALG_SID_SSL3SHAMD5 8
#define ALG_SID_HMAC 9
#define ALG_SID_TLS1PRF 10
#define ALG_SID_SHA_256 12
#define ALG_SID_SHA_384 13
#define ALG_SID_SHA_512 14

#define CALG_MD2 (ALG_CLASS_HASH | ALG_TYPE_ANY | ALG_SID_MD2)
#define CALG_MD4 (ALG_CLASS_HASH | ALG_TYPE_ANY | ALG_SID_MD4)
#define CALG_MD5 (ALG_CLASS_HASH | ALG_TYPE_ANY | ALG_SID_MD5)
#define CALG_SHA (ALG_CLASS_HASH | ALG_TYPE_ANY | ALG_SID_SHA)
#define CALG_SHA1 (ALG_CLASS_HASH | ALG_TYPE_ANY | ALG_SID_SHA1)
#define CALG_SHA_256 (ALG_CLASS_HASH | ALG_TYPE_ANY | ALG_SID_SHA_256)
#define CALG_SHA_384 (ALG_CLASS_HASH | ALG_TYPE_ANY | ALG_SID_SHA_384)
#define CALG_SHA_512 (ALG_CLASS_HASH | ALG_TYPE_ANY | ALG_SID_SHA_512)
#define CALG_MAC (ALG_CLASS_HASH | ALG_TYPE_ANY | ALG_SID_MAC)
#define CALG_HMAC (ALG_CLASS_HASH | ALG_TYPE_ANY | ALG_SID_HMAC)
#define CALG_SSL3_SHAMD5 (ALG_CLASS_HASH | ALG_TYPE_ANY | ALG_SID_SSL3SHAMD5)
#define CALG_TLS1PRF (ALG_CLASS_HASH | ALG_TYPE_ANY | ALG_SID_TLS1PRF)

#define CALG_RSA_SIGN (ALG_CLASS_SIGNATURE | ALG_TYPE_RSA | ALG_SID_RSA_ANY)
#define CALG_RSA_KEYX (ALG_CLASS_KEY_EXCHANGE | ALG_TYPE_RSA | ALG_SID_RSA_ANY)
#define CALG_DSS_SIGN (ALG_CLASS_SIGNATURE | ALG_TYPE_DSS | ALG_SID_DSS_ANY)

#define CALG_DES (ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_BLOCK | ALG_SID_DES)
#define CALG_3DES_112 (ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_BLOCK | ALG_SID_3DES_112)
#define CALG_3DES (ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_BLOCK | ALG_SID_3DES)
#define CALG_DESX (ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_BLOCK | ALG_SID_DESX)
#define CALG_RC2 (ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_BLOCK | ALG_SID_RC2)
#define CALG_RC4 (ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_STREAM | ALG_SID_RC4)
#define CALG_RC5 (ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_BLOCK | ALG_SID_RC5)
#define CALG_AES_128 (ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_BLOCK | ALG_SID_AES_128)
#define CALG_AES_192 (ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_BLOCK | ALG_SID_AES_192)
#define CALG_AES_256 (ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_BLOCK | ALG_SID_AES_256)
#define CALG_AES (ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_BLOCK | ALG_SID_AES)

  /* Key specifications */
#define AT_KEYEXCHANGE 1
#define AT_SIGNATURE 2

  /* Cipher modes */
#define CRYPT_MODE_CBC 1
#define CRYPT_MODE_ECB 2
#define CRYPT_MODE_OFB 3
#define CRYPT_MODE_CFB 4
#define CRYPT_MODE_CTS 5

  /* Hash parameters */
#define HP_ALGID 0x0001
#define HP_HASHVAL 0x0002
#define HP_HASHSIZE 0x0004
#define HP_HMAC_INFO 0x0005

  /* Key parameters */
#define KP_IV 1
#define KP_SALT 2
#define KP_PADDING 3
#define KP_MODE 4
#define KP_MODE_BITS 5
#define KP_PERMISSIONS 6
#define KP_ALGID 7
#define KP_BLOCKLEN 8
#define KP_KEYLEN 9
#define KP_SALT_EX 10
#define KP_P 11
#define KP_G 12
#define KP_Q 13
#define KP_X 14
#define KP_Y 15
#define KP_EFFECTIVE_KEYLEN 19
#define KP_CERTIFICATE 26
#define KP_PRECOMP_MD5 24
#define KP_PRECOMP_SHA 25
#define KP_KEYVAL 30
#define KP_ROUNDS 35

  /* Provider parameters */
#define PP_ENUMALGS 1
#define PP_ENUMCONTAINERS 2
#define PP_IMPTYPE 3
#define PP_NAME 4
#define PP_VERSION 5
#define PP_KEYSET_SEC_DESCR 8
#define PP_CERTCHAIN 9
#define PP_KEYEXCHANGE_ALG 14
#define PP_SIGNATURE_ALG 15
#define PP_PROVTYPE 16
#define PP_ENUMALGS_EX 22
#define PP_UNIQUE_CONTAINER 36

  /* Blob types */
#define SIMPLEBLOB 0x1
#define PUBLICKEYBLOB 0x6
#define PRIVATEKEYBLOB 0x7
#define PLAINTEXTKEYBLOB 0x8
#define OPAQUEKEYBLOB 0x9
#define PUBLICKEYBLOBEX 0xA
#define SYMMETRICWRAPKEYBLOB 0xB
#define KEYSTATEBLOB 0xC

  /* Hash/key enumeration and padding flags */
#define CRYPT_FIRST 1
#define CRYPT_NEXT 2
#define CRYPT_LAST 3
#define CRYPT_MODE_USE_OAEP 0x00000008

  WINADVAPI BOOL WINAPI CryptAcquireContextA (HCRYPTPROV *phProv, LPCSTR szContainer, LPCSTR szProvider, DWORD dwProvType, DWORD dwFlags);
  WINADVAPI BOOL WINAPI CryptAcquireContextW (HCRYPTPROV *phProv, LPCWSTR szContainer, LPCWSTR szProvider, DWORD dwProvType, DWORD dwFlags);
  WINADVAPI BOOL WINAPI CryptReleaseContext (HCRYPTPROV hProv, DWORD dwFlags);
  WINADVAPI BOOL WINAPI CryptGenRandom (HCRYPTPROV hProv, DWORD dwLen, BYTE *pbBuffer);
  WINADVAPI BOOL WINAPI CryptGetProvParam (HCRYPTPROV hProv, DWORD dwParam, BYTE *pbData, DWORD *pdwDataLen, DWORD dwFlags);
  WINADVAPI BOOL WINAPI CryptSetProvParam (HCRYPTPROV hProv, DWORD dwParam, const BYTE *pbData, DWORD dwFlags);
  WINADVAPI BOOL WINAPI CryptEnumProvidersA (DWORD dwIndex, DWORD *pdwReserved, DWORD dwFlags, DWORD *pdwProvType, LPSTR szProvName, DWORD *pcbProvName);
  WINADVAPI BOOL WINAPI CryptEnumProvidersW (DWORD dwIndex, DWORD *pdwReserved, DWORD dwFlags, DWORD *pdwProvType, LPWSTR szProvName, DWORD *pcbProvName);
  WINADVAPI BOOL WINAPI CryptEnumProviderTypesA (DWORD dwIndex, DWORD *pdwReserved, DWORD dwFlags, DWORD *pdwProvType, LPSTR szTypeName, DWORD *pcbTypeName);
  WINADVAPI BOOL WINAPI CryptEnumProviderTypesW (DWORD dwIndex, DWORD *pdwReserved, DWORD dwFlags, DWORD *pdwProvType, LPWSTR szTypeName, DWORD *pcbTypeName);
  WINADVAPI BOOL WINAPI CryptGetDefaultProviderA (DWORD dwProvType, DWORD *pdwReserved, DWORD dwFlags, LPSTR pszProvName, DWORD *pcbProvName);
  WINADVAPI BOOL WINAPI CryptGetDefaultProviderW (DWORD dwProvType, DWORD *pdwReserved, DWORD dwFlags, LPWSTR pszProvName, DWORD *pcbProvName);
  WINADVAPI BOOL WINAPI CryptSetProviderA (LPCSTR pszProvName, DWORD dwProvType);
  WINADVAPI BOOL WINAPI CryptSetProviderW (LPCWSTR pszProvName, DWORD dwProvType);

  WINADVAPI BOOL WINAPI CryptCreateHash (HCRYPTPROV hProv, ALG_ID Algid, HCRYPTKEY hKey, DWORD dwFlags, HCRYPTHASH *phHash);
  WINADVAPI BOOL WINAPI CryptHashData (HCRYPTHASH hHash, const BYTE *pbData, DWORD dwDataLen, DWORD dwFlags);
  WINADVAPI BOOL WINAPI CryptHashSessionKey (HCRYPTHASH hHash, HCRYPTKEY hKey, DWORD dwFlags);
  WINADVAPI BOOL WINAPI CryptDestroyHash (HCRYPTHASH hHash);
  WINADVAPI BOOL WINAPI CryptDuplicateHash (HCRYPTHASH hHash, DWORD *pdwReserved, DWORD dwFlags, HCRYPTHASH *phHash);
  WINADVAPI BOOL WINAPI CryptGetHashParam (HCRYPTHASH hHash, DWORD dwParam, BYTE *pbData, DWORD *pdwDataLen, DWORD dwFlags);
  WINADVAPI BOOL WINAPI CryptSetHashParam (HCRYPTHASH hHash, DWORD dwParam, const BYTE *pbData, DWORD dwFlags);
  WINADVAPI BOOL WINAPI CryptSignHashA (HCRYPTHASH hHash, DWORD dwKeySpec, LPCSTR szDescription, DWORD dwFlags, BYTE *pbSignature, DWORD *pdwSigLen);
  WINADVAPI BOOL WINAPI CryptSignHashW (HCRYPTHASH hHash, DWORD dwKeySpec, LPCWSTR szDescription, DWORD dwFlags, BYTE *pbSignature, DWORD *pdwSigLen);
  WINADVAPI BOOL WINAPI CryptVerifySignatureA (HCRYPTHASH hHash, const BYTE *pbSignature, DWORD dwSigLen, HCRYPTKEY hPubKey, LPCSTR szDescription, DWORD dwFlags);
  WINADVAPI BOOL WINAPI CryptVerifySignatureW (HCRYPTHASH hHash, const BYTE *pbSignature, DWORD dwSigLen, HCRYPTKEY hPubKey, LPCWSTR szDescription, DWORD dwFlags);

  WINADVAPI BOOL WINAPI CryptGenKey (HCRYPTPROV hProv, ALG_ID Algid, DWORD dwFlags, HCRYPTKEY *phKey);
  WINADVAPI BOOL WINAPI CryptDeriveKey (HCRYPTPROV hProv, ALG_ID Algid, HCRYPTHASH hBaseData, DWORD dwFlags, HCRYPTKEY *phKey);
  WINADVAPI BOOL WINAPI CryptDestroyKey (HCRYPTKEY hKey);
  WINADVAPI BOOL WINAPI CryptDuplicateKey (HCRYPTKEY hKey, DWORD *pdwReserved, DWORD dwFlags, HCRYPTKEY *phKey);
  WINADVAPI BOOL WINAPI CryptGetUserKey (HCRYPTPROV hProv, DWORD dwKeySpec, HCRYPTKEY *phUserKey);
  WINADVAPI BOOL WINAPI CryptExportKey (HCRYPTKEY hKey, HCRYPTKEY hExpKey, DWORD dwBlobType, DWORD dwFlags, BYTE *pbData, DWORD *pdwDataLen);
  WINADVAPI BOOL WINAPI CryptImportKey (HCRYPTPROV hProv, const BYTE *pbData, DWORD dwDataLen, HCRYPTKEY hPubKey, DWORD dwFlags, HCRYPTKEY *phKey);
  WINADVAPI BOOL WINAPI CryptGetKeyParam (HCRYPTKEY hKey, DWORD dwParam, BYTE *pbData, DWORD *pdwDataLen, DWORD dwFlags);
  WINADVAPI BOOL WINAPI CryptSetKeyParam (HCRYPTKEY hKey, DWORD dwParam, const BYTE *pbData, DWORD dwFlags);
  WINADVAPI BOOL WINAPI CryptEncrypt (HCRYPTKEY hKey, HCRYPTHASH hHash, BOOL Final, DWORD dwFlags, BYTE *pbData, DWORD *pdwDataLen, DWORD dwBufLen);
  WINADVAPI BOOL WINAPI CryptDecrypt (HCRYPTKEY hKey, HCRYPTHASH hHash, BOOL Final, DWORD dwFlags, BYTE *pbData, DWORD *pdwDataLen);

#ifdef UNICODE
#define CryptAcquireContext CryptAcquireContextW
#define CryptEnumProviders CryptEnumProvidersW
#define CryptEnumProviderTypes CryptEnumProviderTypesW
#define CryptGetDefaultProvider CryptGetDefaultProviderW
#define CryptSetProvider CryptSetProviderW
#define CryptSignHash CryptSignHashW
#define CryptVerifySignature CryptVerifySignatureW
#else
#define CryptAcquireContext CryptAcquireContextA
#define CryptEnumProviders CryptEnumProvidersA
#define CryptEnumProviderTypes CryptEnumProviderTypesA
#define CryptGetDefaultProvider CryptGetDefaultProviderA
#define CryptSetProvider CryptSetProviderA
#define CryptSignHash CryptSignHashA
#define CryptVerifySignature CryptVerifySignatureA
#endif

#ifdef __cplusplus
}
#endif

#endif /* WINAPI_FAMILY_PARTITION (WINAPI_PARTITION_DESKTOP) */

#endif /* _WINCRYPT_H_ */
