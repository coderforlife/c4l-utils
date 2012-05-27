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

#ifndef _ERRORS_H_
#define _ERRORS_H_

// Prints a formatted error message from the error code and the description given
DWORD error(LPCWSTR str, DWORD err);

// Prints a formatted error message from the Win32 last error code and the description given
static inline DWORD last_error(LPCWSTR str) { return error(str, GetLastError()); }

// Prints a formatted error message from the Win32 last error code and the description given
// The first string should contain %s where the second string will be inserted
DWORD last_error_str(LPCWSTR str, LPCWSTR str2);

// Prints a formatted error message from the Win32 last error code and the description given, then closes the given certificate store
DWORD last_error_and_close_store(LPCWSTR str, HCERTSTORE hCertStore);

#endif
