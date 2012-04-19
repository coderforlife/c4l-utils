// This code is for debugging or learning more about different NTFS file streams
// To see all the information about every stream in the files that unblock goes through:
//		Uncomment '#include "WSI-inspector.c"' in unblock.c
//		Uncomment dumpWSI(wsi) in the main loop
// You will then have lots of output on the console describing everything that is easily exposed about each stream
// Start looking for hidden streams everywhere!

#define MIN_STREAM_ID	0x1
#define MAX_STREAM_ID	0xA
static TCHAR *streamIds[] = {
	NULL,
	TEXT("DATA"), TEXT("EA_DATA"), TEXT("SECURITY_DATA"), TEXT("ALTERNATE_DATA"), TEXT("LINK"), TEXT("PROPERTY_DATA"),
	TEXT("OBJECT_ID"), TEXT("REPARSE_DATA"), TEXT("SPARSE_BLOCK"), TEXT("TXFS_DATA")
};

#define MAX_STREAM_ATTR	0x4
static TCHAR *streamAttrs[] = {
	//0				1							2						   4							8
	TEXT("NORMAL"), TEXT("MODIFIED_WHEN_READ"), TEXT("CONTAINS_SECURITY"), TEXT("CONTAINS_PROPERTIES"), TEXT("SPARSE")
};

void printAttr(DWORD attr) {
	int i;
	BOOL first = TRUE;
	if (attr == 0) {
		_tprintf(streamAttrs[0]);
		return;
	}
	for (i = 1; i <= MAX_STREAM_ATTR; ++i) {
		attr >>= 1;
		if (attr & 0x1) {
			if (first) {
				_tprintf(streamAttrs[i]);
				first = FALSE;
			} else {
				_tprintf(TEXT(", %s"), streamAttrs[i]);
			}
		}
	}
	if (attr) {
		_tprintf(first ? TEXT("Unknown: %X") : TEXT(", Unknown: %X"), attr << (MAX_STREAM_ATTR-1));
	}
}

void dumpWSI(WIN32_STREAM_ID *wsi) {
	_tprintf(TEXT("ID:        %s (%X)\n"), ((wsi->dwStreamId<MIN_STREAM_ID||wsi->dwStreamId>MAX_STREAM_ID) ? TEXT("Unknown") : streamIds[wsi->dwStreamId]), wsi->dwStreamId);
	_tprintf(TEXT("Attrs:     ")); printAttr(wsi->dwStreamAttributes); _tprintf(TEXT("\n")); 
	_tprintf(TEXT("Size:     %8I64d\n"), wsi->Size.QuadPart);
	_tprintf(TEXT("Name Size:%8u\n"), wsi->dwStreamNameSize);
	_tprintf(TEXT("Name:      %s\n"), (wsi->dwStreamNameSize > 0) ? wsi->cStreamName : TEXT("[NULL]"));
	_tprintf(TEXT("--------------------\n"));		
}