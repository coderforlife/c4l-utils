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

// The main set of includes and defines

#pragma once

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_SWPRINTFS

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include <WinCrypt.h>
#pragma comment(lib, "Crypt32.lib")
#pragma comment(lib, "Advapi32.lib")

// For PathFindFileName and PathFileExists
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

// Dynamic loading of functions
#define FUNC(n, r, ...) typedef r (WINAPI* FUNC_##n)(__VA_ARGS__); FUNC_##n n = NULL;
#define LOAD_FUNC(n, l) (n = (FUNC_##n)GetProcAddress(l, #n))

// The mingw files don't have all these constants
#ifndef CRYPT_ACQUIRE_ALLOW_NCRYPT_KEY_FLAG
#define CRYPT_ACQUIRE_ALLOW_NCRYPT_KEY_FLAG	0x00010000
#endif
#ifndef CERT_STORE_PROV_REG
#define CERT_STORE_PROV_REG					((LPCSTR)4)
#endif
#ifndef CERT_STORE_BACKUP_RESTORE_FLAG
#define CERT_STORE_BACKUP_RESTORE_FLAG		0x00000800
#endif
#ifndef CERT_STORE_OPEN_EXISTING_FLAG
#define CERT_STORE_OPEN_EXISTING_FLAG		0x00004000
#endif
#ifndef CERT_CLOSE_STORE_FORCE_FLAG
#define CERT_CLOSE_STORE_FORCE_FLAG			0x00000001
#endif

#ifdef _MSC_VER
#define inline __inline
#endif
