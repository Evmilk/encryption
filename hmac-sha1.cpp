// ConsoleApplication2.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <iostream>
#include <windows.h>
#include <wincrypt.h>
#pragma warning(disable:4996)
using namespace std;
#ifndef CALG_HMAC
#define CALG_HMAC (ALG_CLASS_HASH | ALG_TYPE_ANY | ALG_SID_HMAC)
#endif

#ifndef CRYPT_IPSEC_HMAC_KEY
#define CRYPT_IPSEC_HMAC_KEY 0x00000100
#endif
#pragma comment(lib, "crypt32.lib")



char * HMAC(char * str, char * password, DWORD AlgId);


typedef struct _my_blob {
	BLOBHEADER header;
	DWORD len;
	BYTE key[0];
}my_blob;


int main(int argc, _TCHAR* argv[])
{
	string s = "ROSDEVIL";
	string s2 = "password";
	char * data1;
	char * data2;
	data1 = &s[0];
	data2 = &s2[0];
	char * hash_sha1 = HMAC(data1, data2, CALG_SHA1);
	char * hash_md5 = HMAC(data1, data2, CALG_MD5);
	cout << "Hash HMAC-SHA1: " << hash_sha1 << " ( " << strlen(hash_sha1) << " )" << endl;
	cout << "Hash HMAC-MD5: " << hash_md5 << " ( " << strlen(hash_md5) << " )" << endl;

	delete[] hash_sha1;
	delete[] hash_md5;

	cin.get();
	return 0;
}

char * HMAC(char * str, char * password, DWORD AlgId = CALG_MD5) {

	HCRYPTPROV hProv = 0;
	HCRYPTHASH hHash = 0;
	HCRYPTKEY hKey = 0;
	HCRYPTHASH hHmacHash = 0;
	BYTE * pbHash = 0;
	DWORD dwDataLen = 0;
	HMAC_INFO HmacInfo;
	int err = 0;
	ZeroMemory(&HmacInfo, sizeof(HmacInfo));

	if (AlgId == CALG_MD5) {
		HmacInfo.HashAlgid = CALG_MD5;
		pbHash = new BYTE[16];
		dwDataLen = 16;
	}
	else if (AlgId == CALG_SHA1) {
		HmacInfo.HashAlgid = CALG_SHA1;
		pbHash = new BYTE[20];
		dwDataLen = 20;
	}
	else {
		return 0;
	}

	ZeroMemory(pbHash, sizeof(dwDataLen));
	char * res = new char[dwDataLen * 2];

	my_blob * kb = NULL;
	DWORD kbSize = sizeof(my_blob) + strlen(password);

	kb = (my_blob*)malloc(kbSize);
	kb->header.bType = PLAINTEXTKEYBLOB;
	kb->header.bVersion = CUR_BLOB_VERSION;
	kb->header.reserved = 0;
	kb->header.aiKeyAlg = CALG_RC2;
	memcpy(&kb->key, password, strlen(password));
	kb->len = strlen(password);
	if (!CryptAcquireContext(&hProv, NULL, MS_ENHANCED_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_NEWKEYSET)) {
		err = 1;
		goto Exit;
	}


	if (!CryptImportKey(hProv, (BYTE*)kb, kbSize, 0, CRYPT_IPSEC_HMAC_KEY, &hKey)) {
		err = 1;
		goto Exit;
	}

	if (!CryptCreateHash(hProv, CALG_HMAC, hKey, 0, &hHmacHash)) {
		err = 1;
		goto Exit;
	}


	if (!CryptSetHashParam(hHmacHash, HP_HMAC_INFO, (BYTE*)&HmacInfo, 0)) {
		err = 1;
		goto Exit;
	}

	if (!CryptHashData(hHmacHash, (BYTE*)str, strlen(str), 0)) {
		err = 1;
		goto Exit;
	}

	if (!CryptGetHashParam(hHmacHash, HP_HASHVAL, pbHash, &dwDataLen, 0)) {
		err = 1;
		goto Exit;
	}

	ZeroMemory(res, dwDataLen * 2);
	char * temp;
	temp = new char[3];
	ZeroMemory(temp, 3);
	for (unsigned int m = 0; m < dwDataLen; m++) {
		sprintf(temp, "%2x", pbHash[m]);
		if (temp[1] == ' ') temp[1] = '0'; // note these two: they are two CORRECTIONS to the conversion in HEX, sometimes the Zeros are
		if (temp[0] == ' ') temp[0] = '0'; // printed with a space, so we replace spaces with zeros; (this error occurs mainly in HMAC-SHA1)
		sprintf(res, "%s%s", res, temp);
	}

	delete[] temp;

Exit:
	free(kb);
	if (hHmacHash)
		CryptDestroyHash(hHmacHash);
	if (hKey)
		CryptDestroyKey(hKey);
	if (hHash)
		CryptDestroyHash(hHash);
	if (hProv)
		CryptReleaseContext(hProv, 0);


	if (err == 1) {
		delete[] res;
		return 0;
	}

	return res;
}
//Note: using HMAC-MD5 you could perform the famous CRAM-MD5 used to authenticate
//smtp servers.

