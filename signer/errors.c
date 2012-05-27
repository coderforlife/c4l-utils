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

// Functions for printing error messages
// See errors.h for descriptions

#include "stdafx.h"
#include "errors.h"

static inline WCHAR *trim(WCHAR *s) {
	size_t len;
	while (*s && iswspace(*s)) ++s;
	len = wcslen(s);
	while (*s && iswspace(s[--len])) s[len] = 0;
	return s;
}

DWORD error(LPCWSTR str, DWORD err) {
    WCHAR *lpMsgBuf;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&lpMsgBuf, 0, NULL);
	fwprintf(stderr, L"Error: %s: %s (%u)\n", str, trim(lpMsgBuf), err);
    LocalFree(lpMsgBuf);
	return err;
}
DWORD last_error_str(LPCWSTR str, LPCWSTR str2) {
	WCHAR buf[1024];
	_snwprintf(buf, ARRAYSIZE(buf), str, str2);
	return last_error(buf);
}
DWORD last_error_and_close_store(LPCWSTR str, HCERTSTORE hCertStore) {
	DWORD err = last_error(str);
	if (hCertStore) CertCloseStore(hCertStore, 0);
	return err;
}
