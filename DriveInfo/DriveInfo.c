// DriveInfo: Displays detailed drive and partition information for all physical drives in the computer
// Copyright (C) 2011  Jeffrey Bush <jeff@coderforlife.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/gpl.html>.

#define MAX_PARTITIONS	128
#define MAX_DRIVES		64

#define _WIN32_WINNT 0x0501
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <Windows.h>
#include <WinIoCtl.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef ARRAYSIZE
#define ARRAYSIZE(a) sizeof(a)/sizeof(a[0])
#endif

#include "partition-types.h"

// Partition styles
const wchar_t *styles[] = { L"MBR", L"GPT", L"RAW", };

// The units used by byte_units
const wchar_t *units[] = { L"b ", L"KB", L"MB", L"GB", L"TB", };

// Returns the size reduced to one of the units bytes, KB, MB, GB, TB
static double byte_size(ULONGLONG bytes) {
	DWORD i;
	ULONGLONG x;
	for (i = 0, x = 1; i < 4; ++i, x *= 1024)
		if (bytes < x*1024) break;
	return ((double)bytes) / x;
}

// Gets the unit for the reduced size given by byte_size
static const wchar_t *byte_unit(ULONGLONG bytes) {
	DWORD i;
	ULONGLONG x;
	for (i = 0, x = 1024; i < 4; ++i, x *= 1024)
		if (bytes < x) break;
	return units[i];
}

// Prints a partition style
__inline static void print_style(const wchar_t *name, PARTITION_STYLE s) {
	if (s < ARRAYSIZE(styles))
		wprintf(L"%s%s\n", name, styles[s]);
	else
		wprintf(L"%sUnknown (%u)\n", name, s);
}

// Prints a GUID, with an optional description
__inline static void print_guid(const wchar_t *name, GUID g, const wchar_t *desc) {
	wprintf(L"%s{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x} %s\n", name, g.Data1, g.Data2, g.Data3,
		g.Data4[0], g.Data4[1], g.Data4[2], g.Data4[3], g.Data4[4], g.Data4[5], g.Data4[6], g.Data4[7], desc ? desc : L"");
}

// Prints a range of values as a size
__inline static void print_range(const wchar_t *name, LARGE_INTEGER off, LARGE_INTEGER len) {
	if (len.QuadPart == 0)
		wprintf(L"%s0\n", name);
	else
		wprintf(L"%s%6.1f %s %13llu to %13llu\n", name, byte_size(len.QuadPart), byte_unit(len.QuadPart), off.QuadPart, off.QuadPart + len.QuadPart);
}

// Prints a boolean
__inline static void print_bool(const wchar_t *name, BOOLEAN b) {
	wprintf(L"%s%s\n", name, b ? L"Yes" : L"No");
}

// Prints an MBR partition type
__inline static void print_mbr_type(const wchar_t *name, BYTE t, BOOLEAN r) {
	wprintf(L"%s%02hx - %s %s\n", name, (WORD)t, mbr_types[t] ? mbr_types[t] : L"Unknown", r ? L"" : L"(Not Recognized)");
}

// Prints a GPT partition type
__inline static void print_gpt_type(const wchar_t *name, GUID t) {
	size_t j;
	const wchar_t *type = L"Unknown";
	for (j = 0; j < ARRAYSIZE(gpt_guids); ++j) {
		if (IsEqualGUID(&t, gpt_guids[j])) {
			type = gpt_types[j];
			break;
		}
	}
	print_guid(name, t, type);
}

// Prints an attribute, if it exists, and returns an updated attribute value without that value
__inline static DWORD64 print_attr(const wchar_t *name, DWORD64 val, DWORD64 attr) {
	if (attr & val) {
		wprintf(L"               %s\n", name);
		attr &= !val;
	}
	return attr;
}

// Functions for getting the real path of a partition
static BOOL GetWindowsDevicePath(const wchar_t *dos, wchar_t *path) {
	HANDLE sh;
	WCHAR vname[MAX_PATH+1], dname[MAX_PATH+1];
	size_t len;
	BOOL retval = FALSE;

	// Get the first volume (handle is used to iterate)
	sh = FindFirstVolume(vname, ARRAYSIZE(vname));
	if (sh == INVALID_HANDLE_VALUE)
		return FALSE;

	// Iterate through all volumes
	do {
		// Make sure the volume path is valid
		len = wcslen(vname);
		if (len < 5 || wcsncmp(vname, L"\\\\?\\", 4) != 0 || vname[len-1] != L'\\')
			continue;

		// Check the device name (which doesn't work with a trailing '\')
		vname[len-1] = 0;		
		if (QueryDosDeviceW(vname+4, dname, ARRAYSIZE(dname)) && wcscmp(dname, dos) == 0) {
			vname[len-1] = L'\\';
			wcscpy(path, vname);
			retval = TRUE;
			break;
		}
	} while (FindNextVolume(sh, vname, ARRAYSIZE(vname)));

	FindVolumeClose(sh);
	return retval;
}
static BOOL GetWindowsDevicePathFromIndicies(DWORD diskIndex, DWORD partIndex, wchar_t *path) {
	WCHAR dev[MAX_PATH+1], dos[MAX_PATH+1];
	snwprintf(dev, MAX_PATH, L"Harddisk%uPartition%u", diskIndex, partIndex);
	return QueryDosDevice(dev, dos, MAX_PATH) ? GetWindowsDevicePath(dos, path) : FALSE;
}


// Prints all the information about a drive from the given layout
static void print_drive_info(int drivenum, DRIVE_LAYOUT_INFORMATION_EX *layout) {
	DWORD count = layout->PartitionCount, unused = 0, i;
	WCHAR path[MAX_PATH+1] = {0};
	
	wprintf(L"=== Drive %d ==========\n", drivenum);
	print_style(L"Drive Style:   ", (PARTITION_STYLE)layout->PartitionStyle);
	if (layout->PartitionStyle == PARTITION_STYLE_MBR) {
		for (i = 0; i < count; ++i)
			if (layout->PartitionEntry[i].Mbr.PartitionType == PARTITION_ENTRY_UNUSED)
				++unused;
		wprintf(L"Partitions:    %u (%u unused)\n", count, unused);
		wprintf(L"MBR Signature: %lu (0x%08lX)\n", layout->Mbr.Signature, layout->Mbr.Signature);
	} else if (layout->PartitionStyle == PARTITION_STYLE_GPT) {
		for (i = 0; i < count; ++i)
			if (IsEqualGUID(&layout->PartitionEntry[i].Gpt.PartitionType, &PARTITION_ENTRY_UNUSED_GUID))
				++unused;
		wprintf(L"Partitions:    %u\n", count);
		print_guid(L"GPT Disk ID:   ", layout->Gpt.DiskId, NULL);
		print_range(L"GPT Usable:    ", layout->Gpt.StartingUsableOffset, layout->Gpt.UsableLength);
		wprintf(L"GPT Max Parts: %lu\n", layout->Gpt.MaxPartitionCount);
	}

	// Go through each partition
	for (i = 0; i < count; ++i) {
		PARTITION_INFORMATION_EX p = layout->PartitionEntry[i];
		BOOL b;
		if (p.PartitionStyle == PARTITION_STYLE_MBR && p.Mbr.PartitionType == PARTITION_ENTRY_UNUSED ||
			p.PartitionStyle == PARTITION_STYLE_GPT && IsEqualGUID(&p.Gpt.PartitionType, &PARTITION_ENTRY_UNUSED_GUID)) {
			wprintf(L"[Unused Partition]\n");
			continue;
		}

		wprintf(L"Partition %u ----------\n", p.PartitionNumber);(drivenum, p.PartitionNumber, path);
		b = GetWindowsDevicePathFromIndicies(drivenum, p.PartitionNumber, path);
		wprintf(L"  Paths:       %s\n", b ? path : L"[none]");
		if (b) {
			WCHAR paths[1024], *p;
			DWORD cb = 0;
			size_t len = 0;
			if (GetVolumePathNamesForVolumeName(path, paths, ARRAYSIZE(paths), &cb)) {
				for (p = paths; p[0] != 0; p += len + 1) {
					len = wcslen(p);
					wprintf(L"               %s\n", p);
				}
			}
		}
		if (p.PartitionStyle != (PARTITION_STYLE)layout->PartitionStyle)
			print_style(L"  Style:       ", p.PartitionStyle);
		print_range(L"  Size:        ", p.StartingOffset, p.PartitionLength);
		//print_bool(L"  Rewriteable:      ", p.RewritePartition);
		if (p.PartitionStyle == PARTITION_STYLE_MBR) {
			print_mbr_type(L"  MBR Type:    ", (WORD)p.Mbr.PartitionType, p.Mbr.RecognizedPartition);
			print_bool(L"  MBR Boot:    ", p.Mbr.BootIndicator);
			wprintf(L"  MBR Hidden:  %u sectors\n", (WORD)p.Mbr.HiddenSectors);
		} else if (p.PartitionStyle == PARTITION_STYLE_GPT) {
			DWORD64 attr = p.Gpt.Attributes;
			print_gpt_type(L"  GPT Type:    ", p.Gpt.PartitionType);
			print_guid(L"  GPT ID:      ", p.Gpt.PartitionId, NULL);
			wprintf(L"  GPT Attribs: %016llx\n", attr);
			attr = print_attr(L"Platform Required",		GPT_ATTRIBUTE_PLATFORM_REQUIRED,			attr);
			attr = print_attr(L"Data: Read Only",		GPT_BASIC_DATA_ATTRIBUTE_READ_ONLY,			attr);
			attr = print_attr(L"Data: Shadow Copy",		GPT_BASIC_DATA_ATTRIBUTE_SHADOW_COPY,		attr);
			attr = print_attr(L"Data: Hidden",			GPT_BASIC_DATA_ATTRIBUTE_HIDDEN,			attr);
			attr = print_attr(L"Data: No Drive Letter",	GPT_BASIC_DATA_ATTRIBUTE_NO_DRIVE_LETTER,	attr);
			if (attr > 0)
				wprintf(L"               Unknown: %016llx\n", attr);
			wprintf(L"  GPT Name:    %s\n", p.Gpt.Name);
		}
	}

	wprintf(L"\n");
}

// The total amount of memory required for the layout information
#define MEM_SIZE sizeof(DRIVE_LAYOUT_INFORMATION_EX) + MAX_PARTITIONS*sizeof(PARTITION_INFORMATION_EX)

int wmain(int argc, wchar_t* argv[]) {
	HANDLE dev;
	DWORD size;
	BOOL b;
	int i, retval = 0;
	wchar_t drive[32];
	DRIVE_LAYOUT_INFORMATION_EX *layout = (DRIVE_LAYOUT_INFORMATION_EX*)malloc(MEM_SIZE);

	wprintf(TEXT("DriveInfo Copyright (C) 2011  Jeffrey Bush <jeff@coderforlife.com>\n"));
	wprintf(TEXT("This program comes with ABSOLUTELY NO WARRANTY;\n"));
	wprintf(TEXT("This is free software, and you are welcome to redistribute it\n"));
	wprintf(TEXT("under certain conditions;\n"));
	wprintf(TEXT("See http://www.gnu.org/licenses/gpl.html for more details.\n\n"));

	// Go through each drive
	for (i = 0; i < MAX_DRIVES; ++i) {
		// Get the drive name and open it
		snwprintf(drive, ARRAYSIZE(drive), L"\\\\.\\PhysicalDrive%d", i);
		dev = CreateFile(drive, FILE_READ_ACCESS, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (dev == INVALID_HANDLE_VALUE) { continue; } // check if that drive doesn't exist

		// Get the layout information for the drive
		size = MEM_SIZE;
		b = DeviceIoControl(dev, IOCTL_DISK_GET_DRIVE_LAYOUT_EX, NULL, 0, layout, size, &size, NULL);
		CloseHandle(dev);

		// Check for problems
		if (!b) {
			fwprintf(stderr, L"DeviceIoControl failed: %u\n", GetLastError());
			retval = 2;
			break;
		}

		// Print
		print_drive_info(i, layout);
	}

	free(layout);

	return retval;
}
