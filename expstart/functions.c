#include "functions.h"
#include "Memory.h"

LPVOID GetResource(LPCTSTR name, DWORD *size) {
	HRSRC resource;
	HGLOBAL res;
	if (!(resource = FindResource(NULL, name, RT_BITMAP)) || !(res = LoadResource(NULL, resource)))
		return NULL;
	*size = SizeofResource(NULL, resource);
	return LockResource(res);
}

BOOL EnablePriv(LPCTSTR lpszPriv) {
	HANDLE hToken;
	LUID luid;
	TOKEN_PRIVILEGES tkprivs;
	BOOL retval = FALSE;
	ZeroMemory(&tkprivs, sizeof(tkprivs));
	if (!OpenProcessToken(GetCurrentProcess(), (TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY), &hToken))
		return FALSE;
	if (LookupPrivilegeValue(NULL, lpszPriv, &luid)){
		tkprivs.PrivilegeCount = 1;
		tkprivs.Privileges[0].Luid = luid;
		tkprivs.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		retval = AdjustTokenPrivileges(hToken, FALSE, &tkprivs, sizeof(tkprivs), NULL, NULL);
	}
	CloseHandle(hToken);
	return retval;
}

DWORD getRSRCSectionVA(HANDLE proc, BYTE *offset) {
	IMAGE_DOS_HEADER dosh;
	IMAGE_NT_HEADERS nth;
	IMAGE_SECTION_HEADER sect;
	WORD i;

	if (!ReadMem(proc, &dosh, sizeof(dosh), offset))					{ return 0; }
	if (dosh.e_magic != IMAGE_DOS_SIGNATURE)							{ SetLastError(ERROR_INVALID_DATA); return 0; }
	offset += dosh.e_lfanew;

	if (!ReadMem(proc, &nth, sizeof(IMAGE_NT_HEADERS), offset))		{ return 0; }
	if (nth.Signature != IMAGE_NT_SIGNATURE)							{ SetLastError(ERROR_INVALID_DATA); return 0; }
	if (nth.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC)		{ SetLastError(ERROR_INVALID_DATA); return 0; }
	offset += 4+IMAGE_SIZEOF_FILE_HEADER+nth.FileHeader.SizeOfOptionalHeader;

	for (i = 0; i < nth.FileHeader.NumberOfSections; i++) {
		if (!ReadMem(proc, &sect, sizeof(IMAGE_SECTION_HEADER), offset+i*sizeof(IMAGE_SECTION_HEADER))) {
			return 0;
		}
		if (strncmp((CHAR*)sect.Name, ".rsrc", 5) == 0) {
			return sect.VirtualAddress;
		}
	}
	SetLastError(ERROR_NOT_FOUND);
	return 0;
}

DWORD getDirectoryOffset(HANDLE proc, BYTE *offset, LPTSTR id) {
	IMAGE_RESOURCE_DIRECTORY dir;
	IMAGE_RESOURCE_DIRECTORY_ENTRY entry;
	DWORD nEntries, i;

	if (!ReadMem(proc, &dir, sizeof(IMAGE_RESOURCE_DIRECTORY), offset)) { return 0; }

	nEntries = dir.NumberOfIdEntries+dir.NumberOfNamedEntries;

	for (i = 0; i < nEntries; i++) {
		if (!ReadMem(proc, &entry, sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY), offset+sizeof(IMAGE_RESOURCE_DIRECTORY)+i*sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY))) {
			return 0;
		}
		if (MAKEINTRESOURCE(entry.Id) == id) {
			return entry.OffsetToDirectory;
		}
	}
	SetLastError(ERROR_NOT_FOUND);
	return 0;
}

DWORD getFirstDirectoryOffset(HANDLE proc, BYTE *offset) {
	IMAGE_RESOURCE_DIRECTORY dir;
	IMAGE_RESOURCE_DIRECTORY_ENTRY entry;
	if (!ReadMem(proc, &dir, sizeof(IMAGE_RESOURCE_DIRECTORY), offset)) { return 0; }
	if (dir.NumberOfIdEntries+dir.NumberOfNamedEntries < 1) {
		SetLastError(ERROR_EMPTY);
		return 0;
	}
	if (!ReadMem(proc, &entry, sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY), offset+sizeof(IMAGE_RESOURCE_DIRECTORY))) {
		return 0;
	}
	return entry.OffsetToDirectory;
}

BOOL setData(HANDLE proc, BYTE *base, SIZE_T offset, VOID *data, DWORD length) {
	IMAGE_RESOURCE_DATA_ENTRY de;
	if (!ReadMem(proc, &de, sizeof(IMAGE_RESOURCE_DATA_ENTRY), base+offset)) { return FALSE; }
	if (length != de.Size) {
		SetLastError(ERROR_BAD_LENGTH);
		return FALSE;
	}
	return WriteMem(proc, data, length, base+de.OffsetToData);
}

BOOL updateBitmap(HANDLE proc, LPTSTR id, BYTE *base, DWORD rsrcVA, DWORD bitmapOffset, LPVOID data, DWORD size) {
	DWORD nameOffset, langOffset;
	return
		(nameOffset = getDirectoryOffset(proc, base+rsrcVA+bitmapOffset, id)) &&
		(langOffset = getFirstDirectoryOffset(proc, base+rsrcVA+nameOffset)) &&
		setData(proc, base, rsrcVA+langOffset, data, size);
}
