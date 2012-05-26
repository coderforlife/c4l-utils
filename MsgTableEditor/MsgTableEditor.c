// MsgTableEditor: Easily edit an extracted message table
// Copyright (C) 2010-2012  Jeffrey Bush <jeff@coderforlife.com>
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


#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_SWPRINTFS

//#define WIN32_LEAN_AND_MEAN
//#define WIN32_EXTRA_LEAN
//#include <windows.h>

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <tchar.h>

/*

* means done
! means done but not tested


* Q Quit (EOF also works)
* H Help (? also works)
* D Display Message Table

* S Save
* A Save As
* L Load
* N New File

* R Replace Message
! M Add Message
! B Add Block
! X Delete Message
! W Delete Block
! I Set ID Base

*/

#define MAX_WORD		65535
#define MAX_DWORD		18446744073709551615
#define MAX_TEXT_LEN	((MAX_WORD-2*sizeof(uint16_t))/sizeof(TCHAR))
#define MAX_PATH		260

#ifdef _UNICODE
#define UNI	true
#else
#define UNI false
#endif

#if defined(_MSC_VER) && _MSC_VER <= 1500
typedef unsigned __int8 uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
typedef __int8 int8_t;
typedef __int16 int16_t;
typedef __int32 int32_t;
typedef __int64 int64_t;
#else
#include <stdint.h>
#endif

typedef unsigned char byte;

#ifndef __cplusplus
typedef uint8_t bool;
#define true  1
#define false 0
#endif

#ifndef ARRAYSIZE
#define ARRAYSIZE(a) sizeof(a)/sizeof(a[0])
#endif

// Raw Message Resource Data
typedef struct _MESSAGE_RESOURCE_ENTRY {
	uint16_t Length;
	uint16_t Flags;
	byte     Text[ 1 ];
} MESSAGE_RESOURCE_ENTRY, *PMESSAGE_RESOURCE_ENTRY;
#define MESSAGE_RESOURCE_UNICODE 0x0001
typedef struct _MESSAGE_RESOURCE_BLOCK {
	uint32_t LowId;
	uint32_t HighId;
	uint32_t OffsetToEntries;
} MESSAGE_RESOURCE_BLOCK, *PMESSAGE_RESOURCE_BLOCK;
typedef struct _MESSAGE_RESOURCE_DATA {
	uint32_t NumberOfBlocks;
	MESSAGE_RESOURCE_BLOCK Blocks[ 1 ];
} MESSAGE_RESOURCE_DATA, *PMESSAGE_RESOURCE_DATA;

// In-memory Message Resource Data
typedef struct _MT_ENTRY {
	uint32_t id;
	bool unicode;
	union {
		wchar_t *wText;
		char    *aText;
		TCHAR   *tText;
	};
} MT_ENTRY;
typedef struct _MT_BLOCK {
	uint32_t lowId;
	uint32_t highId;
	uint32_t nEntries;
	MT_ENTRY **entries;
} MT_BLOCK;
typedef struct _MT_DATA {
	TCHAR *file;
	uint32_t nBlocks;
	MT_BLOCK **blocks;
} MT_DATA;

static wchar_t *CopyStrW(wchar_t *s) { return wcscpy((wchar_t*)malloc(sizeof(wchar_t)*(wcslen(s)+1)), s); }
static char    *CopyStrA(char    *s) { return strcpy((char   *)malloc(sizeof(char   )*(strlen(s)+1)), s); }
#ifdef _UNICODE
#define CopyStr(s) CopyStrW(s)
#else
#define CopyStr(s) CopyStrA(s)
#endif

static wchar_t *GetStringW(wchar_t *s) {
	size_t i = wcslen(s)+1;
	bool end = true;
	wchar_t *x = (wchar_t*)malloc(sizeof(wchar_t)*i);
	do {
		--i;
		x[i] = (end && (end = (s[i] == 0 || s[i] == L'\n' || s[i] == L'\r'))) ? 0: s[i];
	} while (i);
	return x;
}

static char *GetStringA(char *s) {
	size_t i = strlen(s)+1;
	bool end = true;
	char *x = (char*)malloc(sizeof(char)*i);
	do {
		--i;
		x[i] = (end && (end = (s[i] == 0 || s[i] == '\n' || s[i] == '\r'))) ? 0: s[i];
	} while (i);
	return x;
}

static MT_DATA *CreateData(TCHAR *file, uint32_t nBlocks) {
	MT_DATA *d = (MT_DATA*)malloc(sizeof(MT_DATA));
	d->file = file;
	d->nBlocks = nBlocks;
	d->blocks = (MT_BLOCK**)memset(malloc(sizeof(MT_BLOCK*)*nBlocks), 0, sizeof(MT_BLOCK*)*nBlocks);
	return d;
}

static MT_BLOCK *CreateBlock(uint32_t lowId, uint32_t highId) {
	MT_BLOCK *b = (MT_BLOCK*)malloc(sizeof(MT_BLOCK));
	b->lowId = lowId;
	b->highId = highId;
	b->nEntries = highId-lowId+1;
	b->entries = (MT_ENTRY**)memset(malloc(sizeof(MT_ENTRY*)*b->nEntries), 0, sizeof(MT_ENTRY*)*b->nEntries);
	return b;
}

static MT_ENTRY *CreateEntry(uint32_t id, bool unicode) {
	MT_ENTRY *e = (MT_ENTRY*)malloc(sizeof(MT_ENTRY));
	e->id = id;
	e->unicode = unicode;
	e->aText = NULL;
	return e;
}

static MT_DATA *BuildData(TCHAR *file, byte *x) {
	uint32_t i, j, offset;
	MESSAGE_RESOURCE_DATA *data = (MESSAGE_RESOURCE_DATA*)x;

	MT_DATA *d = CreateData(file, data->NumberOfBlocks);

	for (i = 0; i < d->nBlocks; ++i) {
		MESSAGE_RESOURCE_BLOCK *block = data->Blocks+i;
		d->blocks[i] = CreateBlock(block->LowId, block->HighId);
		offset = block->OffsetToEntries;
		for (j = 0; j < d->blocks[i]->nEntries; ++j) {
			MESSAGE_RESOURCE_ENTRY *entry = (MESSAGE_RESOURCE_ENTRY*)(x+offset);
			d->blocks[i]->entries[j] = CreateEntry(j+block->LowId, (entry->Flags & MESSAGE_RESOURCE_UNICODE) == MESSAGE_RESOURCE_UNICODE);
			if (d->blocks[i]->entries[j]->unicode) {
				d->blocks[i]->entries[j]->wText = GetStringW((wchar_t*)entry->Text);
			} else {
				d->blocks[i]->entries[j]->aText = GetStringA((char   *)entry->Text);
			}
			offset += entry->Length;
		}
	}
	
	return d;
}

static MT_DATA *ReadMessageTable(TCHAR *file) {
	size_t size, read;
	byte *data = NULL;
	FILE* f = _tfopen(file, _T("rb"));
	if (f == NULL) {
		_ftprintf(stderr, _T("! Could not open file '%s': %u\n"), file, errno);
		return NULL;
	}
	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fseek(f, 0, SEEK_SET);
	if (size == -1) {
		_ftprintf(stderr, _T("! Unable to get file size: %u\n"), errno);
	} else if ((data = (byte*)malloc(size)) == NULL) {
		_ftprintf(stderr, _T("! Unable to allocate memory: %u\n"), errno);
	} else if ((read = fread(data, 1, size, f)) != size) {
		_ftprintf(stderr, _T("! Didn't read all bytes: %u!=%u\n"), read, size);
		free(data);
		data = NULL;
	}
	fclose(f);
	if (data) {
		MT_DATA *x = BuildData(file, data);
		free(data);
		return x;
	}
	return NULL;
}

static MT_DATA *CreateBlankMessageTable() {
	MT_DATA *data = CreateData(NULL, 1);
	data->blocks[0] = CreateBlock(1, 1);
	data->blocks[0]->entries[0] = CreateEntry(1, true);
	data->blocks[0]->entries[0]->tText = CopyStr(_T("Example Message"));
	return data;
}

static TCHAR *ReadLine(TCHAR *s, int size) {
	size_t i;
	if (_fgetts(s, size, stdin) == NULL) {
		return NULL;
	}
	for (i = _tcslen(s)-1; i > 1 && (s[i] == _T('\r') || s[i] == _T('\n')); --i) {
		s[i] = 0;
	}
	return s;
}

static int ReadChar() {
	char temp[128];
	if (fgets(temp, ARRAYSIZE(temp), stdin) == NULL) {
		return EOF;
	}
	return (int)temp[0];
}

static uint16_t align4(uint16_t x) {
	return x + x % 4;
}

static byte *CompileData(MT_DATA *d, size_t *size) {
	uint32_t i, j, off = d->nBlocks*sizeof(MESSAGE_RESOURCE_BLOCK)+sizeof(uint32_t);
	size_t sz;
	byte *bytes;
	MESSAGE_RESOURCE_DATA *data;

	sz = off;
	for (i = 0; i < d->nBlocks; ++i) {
		sz += 2*d->blocks[i]->nEntries*sizeof(uint16_t);
		for (j = 0; j < d->blocks[i]->nEntries; ++j) {
			MT_ENTRY *e = d->blocks[i]->entries[j];
			if (e->unicode) {
				sz += align4((uint16_t)(sizeof(wchar_t)*(wcslen(e->wText)+3))); // + 3 for 0D 0A 00
			} else {
				sz += align4((uint16_t)(sizeof(char   )*(strlen(e->aText)+3)));
			}
		}
	}
	*size = sz;
	bytes = (byte*)memset(malloc(sz), 0, sz);
	data = (MESSAGE_RESOURCE_DATA*)bytes;
	data->NumberOfBlocks = d->nBlocks;
	for (i = 0; i < d->nBlocks; ++i) {
		data->Blocks[i].LowId = d->blocks[i]->lowId;
		data->Blocks[i].HighId = d->blocks[i]->highId;
		data->Blocks[i].OffsetToEntries = off;
		for (j = 0; j < d->blocks[i]->nEntries; ++j) {
			MESSAGE_RESOURCE_ENTRY *entry = (MESSAGE_RESOURCE_ENTRY*)(bytes+off);
			MT_ENTRY *e = d->blocks[i]->entries[j];
			uint16_t sz;
			if (e->unicode) {
				sz = (uint16_t)((wcslen(e->wText)+3)*sizeof(wchar_t));
				entry->Flags = MESSAGE_RESOURCE_UNICODE;
				wcscat(wcscpy((wchar_t*)entry->Text, e->wText), L"\r\n\0");
			} else {
				sz = (uint16_t)((strlen(e->aText)+3)*sizeof(char));
				entry->Flags = 0;
				strcat(strcpy((char   *)entry->Text, e->aText), "\r\n\0");
			}
			entry->Length = align4(sz)+2*sizeof(uint16_t);
			off += entry->Length;
		}
	}

	return bytes;
}

static bool SaveMessageTable(MT_DATA *data, bool always_ask_for_filename) {
	byte *bytes;
	size_t size, wrote;
	bool retval = false;
	FILE* f;

	if (always_ask_for_filename || !data->file) {
		TCHAR file[MAX_PATH];
		_tprintf(_T("Save to: "));
		ReadLine(file, MAX_PATH);
		if (data->file)
			free(data->file);
		data->file = CopyStr(file);
	}

	bytes = CompileData(data, &size);
	f = _tfopen(data->file, _T("wb"));
	if (f == NULL) {
		_ftprintf(stderr, _T("! Could not open file '%s': %u\n"), data->file, errno);
	} else if ((wrote = fwrite(bytes, 1, size, f)) != size) {
		_ftprintf(stderr, _T("! Didn't write all bytes: %u!=%u\n"), wrote, size);
	} else {
		retval = true;
	}
	free(bytes);
	fclose(f);
	if (retval)
		_tprintf(_T("Successfully saved to %s\n"), data->file);
	return retval;
}

static bool AskToSave(MT_DATA *data) {
	_tprintf(_T("Current data not saved, do you wish to save now? [Y/N/X] "));
	for (;;) {
		switch (toupper(ReadChar())) {
		case 'Y': return SaveMessageTable(data, false);
		case 'N': return true;
		case 'X': return false;
		default:  _tprintf(_T("Please enter Y to save, N to not save, or X to cancel: "));
		}
	}
}

static MT_ENTRY *GetEntry(MT_BLOCK *block, uint32_t id) {
	return (id < block->lowId || id > block->highId) ? NULL : block->entries[id-block->lowId];
}

static void DestroyEntry(MT_ENTRY *entry) {
	free(entry->wText);
	entry->wText = NULL;
	free(entry);
}

static void DestroyBlock(MT_BLOCK *block) {
	uint32_t i;
	for (i = 0; i < block->nEntries; ++i) {
		DestroyEntry(block->entries[i]);
		block->entries[i] = NULL;
	}
	free(block->entries);
	block->entries = NULL;
	free(block);
}

static void DestroyData(MT_DATA *data) {
	uint32_t i;
	for (i = 0; i < data->nBlocks; ++i) {
		DestroyBlock(data->blocks[i]);
		data->blocks[i] = NULL;
	}
	free(data->blocks);
	data->blocks = NULL;
	free(data->file);
	data->file = NULL;
}

static void PrintText(MT_ENTRY *entry) {
	if (entry->unicode) {
		wprintf(L"\"%s\"\n", entry->wText);
	} else {
		printf("\"%s\"\n", entry->aText);
	}
}

static void DisplayMessageTable(MT_DATA *data) {
	uint32_t i, j;
	_tprintf(_T("File: %s\n"), data->file);
	for (i = 0; i < data->nBlocks; ++i) {
		_tprintf(_T("Block: %u\n"), i);
		for (j = 0; j < data->blocks[i]->nEntries; ++j) {
			wprintf(L"  %u: ", data->blocks[i]->entries[j]->id);
			PrintText(data->blocks[i]->entries[j]);
		}
	}
}

static uint32_t AskBlockId(MT_DATA *data, bool insert) {
	const uint32_t max_b_id = insert ? data->nBlocks : data->nBlocks - 1;
	uint32_t b_id;
	if (max_b_id == 0) {
		_tprintf(_T("Selected Block 0 (only 1 block to select)\n"));
		b_id = 0;
	} else {
		_tprintf(_T("%s (0-%u): "), insert ? _T("New block id") : _T("Block"), max_b_id);
		_tscanf(_T("%u"), &b_id);
		while (b_id > max_b_id) {
			_tprintf(_T("Invalid block id, valid choices are 0-%u: "), max_b_id);
			_tscanf(_T("%u"), &b_id);
		}
		ReadChar(); // clears the new line character
	}
	return b_id;
}

static uint32_t AskEntryId(MT_BLOCK *block, bool insert) {
	const uint32_t max_e_high_id = insert ? block->highId+1 : block->highId;
	uint32_t e_id;
	if (block->lowId == max_e_high_id) {
		_tprintf(_T("Selected Msg ID %u (only 1 msg id to select)\n"), block->lowId);
		e_id = block->lowId;
	} else {
		_tprintf(_T("%s (%u-%u): "), insert ? _T("New msg ID") : _T("Msg ID"), block->lowId, max_e_high_id);
		_tscanf(_T("%u"), &e_id);
		while (e_id < block->lowId || e_id > max_e_high_id) {
			_tprintf(_T("Invalid msg ID, valid choices are %u-%u: "), block->lowId, max_e_high_id);
			_tscanf(_T("%u"), &e_id);
		}
		ReadChar(); // clears the new line character
	}
	return e_id;
}

static bool AskIfSure(const TCHAR* type) {
	_tprintf(_T("Are you sure you want to delete this %s? [Y/N] "), type);
	for (;;) {
		switch (toupper(ReadChar())) {
		case 'Y': return true;
		case 'N': return false;
		default:  _tprintf(_T("Please enter Y to delete this %s and N to cancel"), type);
		}
	}
}

static void UpdateEntryIds(MT_BLOCK *block) {
	uint32_t id, i;
	block->highId = block->lowId+block->nEntries-1;
	for (id = block->lowId, i = 0; i < block->nEntries; ++id, ++i) {
		block->entries[i]->id = id;
	}
}

static void ReplaceMessage(MT_DATA *data) {
	uint32_t b_id = AskBlockId(data, false);
	MT_BLOCK *block = data->blocks[b_id];
	uint32_t e_id = AskEntryId(block, false);
	MT_ENTRY *entry = GetEntry(block, e_id);
	TCHAR text[MAX_TEXT_LEN] = {0};
	_tprintf(_T("You selected:\n%u:%u: "), b_id, e_id);
	PrintText(entry);
	free(entry->tText);
	_tprintf(_T("New text: "));
	ReadLine(text, MAX_TEXT_LEN);
	entry->unicode = UNI;
	entry->tText = CopyStr(text);
	_tprintf(_T("Set text to:\n%u:%u: "), b_id, e_id);
	PrintText(entry);
}

static void AddMessage(MT_DATA *data) {
	uint32_t b_id = AskBlockId(data, false);
	MT_BLOCK *block = data->blocks[b_id];
	uint32_t e_id = AskEntryId(block, true);
	uint32_t i = e_id-block->lowId;
	TCHAR text[MAX_TEXT_LEN] = {0};
	_tprintf(_T("Message text: "));
	ReadLine(text, MAX_TEXT_LEN);
	block->entries = (MT_ENTRY**)realloc(block->entries, ++block->nEntries*sizeof(MT_ENTRY*));
	memmove(block->entries+i+1, block->entries+i, (block->nEntries-i-1)*sizeof(MT_ENTRY*));
	block->entries[i] = CreateEntry(0, UNI);
	block->entries[i]->tText = CopyStr(text);
	UpdateEntryIds(block);
	_tprintf(_T("Added new message:\n%u:%u: "), b_id, e_id);
	PrintText(block->entries[i]);
}

static void AddBlock(MT_DATA *data) {
	uint32_t b_id = AskBlockId(data, true), id;
	TCHAR text[MAX_TEXT_LEN] = {0};
	_tprintf(_T("Message ID base: "));
	_tscanf(_T("%u"), &id);
	ReadChar(); // clears the new line character
	_tprintf(_T("Message text: "));
	ReadLine(text, MAX_TEXT_LEN);
	data->blocks = (MT_BLOCK**)realloc(data->blocks, ++data->nBlocks*sizeof(MT_BLOCK*));
	memmove(data->blocks+b_id+1, data->blocks+b_id, (data->nBlocks-b_id-1)*sizeof(MT_BLOCK*));
	data->blocks[b_id] = CreateBlock(id, id);
	data->blocks[b_id]->entries[0] = CreateEntry(id, UNI);
	data->blocks[b_id]->entries[0]->tText = CopyStr(text);
	_tprintf(_T("Added new block: %u\n"), b_id);
}

static void DeleteMessage(MT_DATA *data) {
	uint32_t b_id = AskBlockId(data, false);
	MT_BLOCK *block = data->blocks[b_id];
	if (block->nEntries <= 1) {
		_tprintf(_T("You cannot delete the last message of a block\n"));
	} else {
		uint32_t e_id = AskEntryId(block, false);
		uint32_t i = e_id-block->lowId;
		if (AskIfSure(_T("message"))) {
			DestroyEntry(block->entries[i]);
			memmove(block->entries+i, block->entries+i+1, (--block->nEntries-i)*sizeof(MT_ENTRY*));
			block->entries[block->nEntries] = NULL;
			block->entries = (MT_ENTRY**)realloc(block->entries, block->nEntries*sizeof(MT_ENTRY*));
			UpdateEntryIds(block);
			_tprintf(_T("Removed message %u:%u\n"), b_id, e_id);
		}
	}
}

static void DeleteBlock(MT_DATA *data) {
	if (data->nBlocks <= 1) {
		_tprintf(_T("You cannot delete the last block\n"));
	} else {
		uint32_t b_id = AskBlockId(data, false);
		if (AskIfSure(_T("block"))) {
			DestroyBlock(data->blocks[b_id]);
			memmove(data->blocks+b_id, data->blocks+b_id+1, (--data->nBlocks-b_id)*sizeof(MT_BLOCK*));
			data->blocks[data->nBlocks] = NULL;
			data->blocks = (MT_BLOCK**)realloc(data->blocks, data->nBlocks*sizeof(MT_BLOCK*));
			_tprintf(_T("Removed block %u\n"), b_id);
		}
	}
}

static void SetIdBase(MT_DATA *data) {
	uint32_t b_id = AskBlockId(data, false), id;
	MT_BLOCK *block = data->blocks[b_id];
	_tprintf(_T("Current ID base is %u so message IDs range from %u to %u\n"), block->lowId, block->lowId, block->highId);
	_tprintf(_T("New ID base: "));
	_tscanf(_T("%u"), &id);
	ReadChar(); // clears the new line character
	block->lowId = id;
	UpdateEntryIds(block);
	_tprintf(_T("New message ID range: %u to %u\n"), block->lowId, block->highId);
}

int _tmain(int argc, _TCHAR* argv[]) {
	MT_DATA *data;
	bool saved = true;

	_tprintf(_T("MsgTableEditor Copyright (C) 2010-2012  Jeffrey Bush <jeff@coderforlife.com>\n"));
	_tprintf(_T("This program comes with ABSOLUTELY NO WARRANTY;\n"));
	_tprintf(_T("This is free software, and you are welcome to redistribute it\n"));
	_tprintf(_T("under certain conditions;\n"));
	_tprintf(_T("See http://www.gnu.org/licenses/gpl.html for more details.\n\n"));

	if (argc > 2) {
		_tprintf(_T("Usage: [file.msg]\n"));
		_tprintf(_T("Note: this is simply the extracted message table, not a PE file with a message table\n"));
		return 1;
	}

	if (argc == 1) {
		data = CreateBlankMessageTable();
	} else {
		data = ReadMessageTable(CopyStr(argv[1]));
		if (!data) { data = CreateBlankMessageTable(); }
	}

	DisplayMessageTable(data);

	for(;;) {
		int c;
		_tprintf(_T("\nCommand (H for help): "));
		c = toupper(ReadChar());
		if (c == EOF || c == 'Q') { if (c == EOF || saved || AskToSave(data)) { break; } }
		else if (c == 'H' || c == '?') { _tprintf(_T("  Q Quit\n  H Help\n  D Display Message Table\n\n  S Save\n  A Save As\n  L Load\n  N New File\n\n  R Replace Message\n  M Add Message\n  B Add Block\n  X Delete Message\n  W Delete Block\n  I Set ID Base\n")); }
		else if (c == 'D') { DisplayMessageTable(data); }
		else if (c == 'R') { ReplaceMessage(data);	saved = false; }
		else if (c == 'M') { AddMessage(data);		saved = false; }
		else if (c == 'B') { AddBlock(data);		saved = false; }
		else if (c == 'X') { DeleteMessage(data);	saved = false; }
		else if (c == 'W') { DeleteBlock(data);		saved = false; }
		else if (c == 'I') { SetIdBase(data);		saved = false; }
		else if (c == 'S') { SaveMessageTable(data, false); saved = true; }
		else if (c == 'A') { SaveMessageTable(data, true);  saved = true; }
		else if (c == 'L') {
			MT_DATA *data2;
			TCHAR file[MAX_PATH];
			if (!saved) { saved = AskToSave(data); }
			if (saved) {
				_tprintf(_T("Load Filename: "));
				ReadLine(file, MAX_PATH);
				data2 = ReadMessageTable(CopyStr(file));
				if (data2) {
					DestroyData(data);
					data = data2;
					saved = true;
				}
			}
		} else if (c == 'N') {
			if (!saved) { saved = AskToSave(data); }
			if (saved) {
				DestroyData(data);
				data = CreateBlankMessageTable();
				saved = true;
			}
		} else {
			_tprintf(_T("Command not understood: %c\n"), (char)c);
		}
	}

	DestroyData(data);

	return 0;
}
