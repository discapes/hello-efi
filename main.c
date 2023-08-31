#include <efi.h>
#include <stdbool.h>
#include "libk.h"
#include "bytemaps/noto_16.h"

#define errorCheck(actual, expected) \
	if (actual != expected)          \
	return actual
#define successCheck(actual)   \
	if (actual != EFI_SUCCESS) \
	return actual
#define printCon(s) systemTable->ConOut->OutputString(systemTable->ConOut, s)
#define printlnCon() systemTable->ConOut->OutputString(systemTable->ConOut, L"\r\n")

char *EFI_MEMORY_TYPE_STRINGS[] = {
	"EfiReservedMemoryType",
	"EfiLoaderCode",
	"EfiLoaderData",
	"EfiBootServicesCode",
	"EfiBootServicesData",
	"EfiRuntimeServicesCode",
	"EfiRuntimeServicesData",
	"EfiConventionalMemory",
	"EfiUnusableMemory",
	"EfiACPIReclaimMemory",
	"EfiACPIMemoryNVS",
	"EfiMemoryMappedIO",
	"EfiMemoryMappedIOPortSpace",
	"EfiPalCode",
	"EfiPersistentMemory",
};

wchar_t *EFI_GRAPHICS_PIXEL_FORMAT_STRINGS[] = {
	L"PixelRedGreenBlueReserved8BitPerColor",
	L"PixelBlueGreenRedReserved8BitPerColor",
	L"PixelBitMask",
	L"PixelBltOnly",
	L"PixelFormatMa"};

bool isUsableMem(UINT32 descType)
{
	return descType == EfiBootServicesCode ||
		   descType == EfiBootServicesData ||
		   descType == EfiConventionalMemory;
}

struct screen
{
	// width, height, pixelwidth, framebuffersize, framebufferbase
	i64 w, h, pixw, fbs;
	u8 *fbb;

	// i.e. where does the next char go? - size of line and col is a character
	i64 line, col;

	u8 *font;
	i64 fontW, fontH;
} screen;

struct mmap
{
	i64 size, descSize;
	// u8 because the stride might not be sizeof(EFI_MEMORY_DESCRIPTOR)
	u8 *map;
} mmap;

int _fltused = 0;

void print(char *str)
{
	for (i64 i = 0; i < 999; i++)
	{
		if (str[i] == '\n')
		{
			screen.line++;
			screen.col = 1;
		}
		else if (str[i] == '\0')
		{
			return;
		}
		else
		{
			// stride = how many bytes to get to the next instance

			// char = bitmap character
			i64 charStride = screen.fontH * screen.fontW;
			i64 charIndex = str[i] - ' '; // printable chars start at 32 (space).
			i64 charOffset = charIndex * charStride;

			i64 lineStride = screen.w * screen.pixw * screen.fontH;
			i64 colStride = screen.pixw * screen.fontW;
			u8 *screenOffset = screen.fbb + (screen.line * lineStride) + (screen.col * colStride);

			i64 xStride = screen.w * screen.pixw;
			i64 yStride = screen.pixw;
			for (int x = 0; x < screen.fontH; x++)
			{
				for (int y = 0; y < screen.fontW; y++)
				{
					u8 *pix = screenOffset + x * xStride + y * yStride;
					double opacity = (double)(255 - screen.font[charOffset + x * screen.fontW + y]) / 255.;
					pix[0] = pix[0] * opacity;
					pix[1] = pix[1] * opacity;
					pix[2] = pix[2] * opacity;
				}
			}
			screen.col++;
		}
	}
}

void println()
{
	print("\n");
}

void printScreenInfo()
{
	print("Video: ");
	print("res = ");
	print(itoa(screen.w));
	print("x");
	print(itoa(screen.h));
	print(" | addr = 0x");
	print(itoa_base((i64)screen.fbb, 16));
	print(" | size = ");
	print(itoa(screen.fbs));
	println();
}

EFI_STATUS initScreen(EFI_SYSTEM_TABLE *systemTable)
{
	EFI_STATUS result;
	EFI_GUID gopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
	EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
	result = systemTable->BootServices->LocateProtocol(&gopGuid, NULL, (void **)&gop);
	successCheck(result);
	printCon(L"Got graphics output protocol, listing video modes...\r\n");

	for (i64 i = 0; i < gop->Mode->MaxMode; i++)
	{
		EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;
		UINTN infoSize;

		result = gop->QueryMode(gop, i, &infoSize, &info);
		printCon(witoa(i));
		printCon(L": ");
		printCon(witoa(info->HorizontalResolution));
		printCon(L"x");
		printCon(witoa(info->VerticalResolution));
		printCon(L" [");
		printCon(witoa(info->PixelsPerScanLine));
		printCon(L" pps]");
		printCon(L" [format ");
		printCon(EFI_GRAPHICS_PIXEL_FORMAT_STRINGS[info->PixelFormat]);
		printCon(L"]");

		if (i == gop->Mode->Mode)
		{
			printCon(L" (current)");
			screen.w = info->HorizontalResolution;
			screen.h = info->VerticalResolution;
			screen.fbb = (u8 *)gop->Mode->FrameBufferBase;
			screen.fbs = gop->Mode->FrameBufferSize;
			screen.pixw = 4;
			screen.col = 1;
			screen.line = 1;
			screen.fontH = 16;
			screen.fontW = 9;
			screen.font = noto_16_map;
		}
		printlnCon();
	}
	printCon(L"Listed video modes.\r\n");
	return EFI_SUCCESS;
}

EFI_STATUS initMmapAndExit(EFI_SYSTEM_TABLE *systemTable, EFI_HANDLE imageHandle)
{
	EFI_STATUS result;
	UINTN mapSize = 0;
	UINTN mapKey;
	UINTN descriptorSize;
	UINT32 descriptorVersion;
	EFI_MEMORY_DESCRIPTOR *memoryMap;

	result = systemTable->BootServices->GetMemoryMap(&mapSize, memoryMap, &mapKey, &descriptorSize, &descriptorVersion);
	errorCheck(result, EFI_BUFFER_TOO_SMALL);

	mapSize += 2 * descriptorSize;
	UINTN bufSize = mapSize;
	result = systemTable->BootServices->AllocatePool(EfiLoaderData, mapSize, (void **)&memoryMap);
	successCheck(result);

	result = systemTable->BootServices->GetMemoryMap(&mapSize, memoryMap, &mapKey, &descriptorSize, &descriptorVersion);
	successCheck(result);
	printCon(L"Got memory map.\r\n");

	mmap.descSize = descriptorSize;
	mmap.map = (u8 *)memoryMap;
	mmap.size = mapSize;

	printCon(L"Leaving boot services!\r\n");
	// https://uefi.org/specs/UEFI/2.9_A/07_Services_Boot_Services.html#efi-boot-services-exitbootservices
	// https://forum.osdev.org/viewtopic.php?f=1&t=42288
	// the first call might not work
	while ((result = systemTable->BootServices->ExitBootServices(imageHandle, mapKey)) == EFI_INVALID_PARAMETER)
	{
		mapSize = bufSize;
		result = systemTable->BootServices->GetMemoryMap(&mapSize, memoryMap, &mapKey, &descriptorSize, &descriptorVersion);
		successCheck(result);
	}

	return EFI_SUCCESS;
}

void drawBackground()
{
	for (i64 i = 0; i < screen.fbs; i += 4)
	{
		u8 *pix = screen.fbb + i;
		pix[0] = 255;
		pix[1] = 200;
		pix[2] = (255 * i) / screen.fbs;
	}
}

void printMmapInfo()
{
	int printedI = 0;
	int n = mmap.size / mmap.descSize;
	int usableMemPartSize = 0;
	int usableMemTotalSize = 0;
	for (int i = 0; i < n; i++)
	{
		EFI_MEMORY_DESCRIPTOR *desc = (EFI_MEMORY_DESCRIPTOR *)(mmap.map + i * mmap.descSize);

		bool usable = isUsableMem(desc->Type);
		if (usable)
		{
			usableMemPartSize += desc->NumberOfPages * 4;
			usableMemTotalSize += desc->NumberOfPages * 4;
		}
		else
		{
			if (usableMemPartSize > 0)
			{
				print(itoa(printedI++));
				print(": --- - ");
				print(itoa(usableMemPartSize));
				print(" KiB\n");
				usableMemPartSize = 0;
			}
			print(itoa(printedI++));
			print(": ");
			print(EFI_MEMORY_TYPE_STRINGS[desc->Type]);
			print(" - ");
			print(itoa(desc->NumberOfPages * 4));
			print(" KiB\n");
		}
	}
	print("Total usable mem size ");
	print(itoa(usableMemTotalSize));
	print(" KiB\n");
}

struct Region
{
	u8 *baseAddress;
	i64 nPages;
};

struct RegionListCursor
{
	i64 region;
	i64 page;
	i64 totalPagesUsed;
};

i64 PAGESIZE = 4096;

u8 *getNextAvailablePage(struct RegionListCursor cursor, struct Region *availableRegions)
{
	struct Region currentRegion = availableRegions[cursor.region];
	u8 *pageAddress = currentRegion.baseAddress + cursor.page + PAGESIZE;
	cursor.totalPagesUsed++;

	if (cursor.page == currentRegion.nPages)
	{
		// this was the last page, move on
		cursor.page = 0;
		cursor.region++;
	}
	else
	{
		cursor.page++;
	}
	return pageAddress;
}

#pragma pack(1)

struct mapping_table
{
	uint64_t entries[512];
};

#pragma pack()

__attribute__((aligned(4096))) struct mapping_table pml4;

void identityMapPages()
{
	// these 4 are calculated from the memory map
	struct Region availableRegions[1000];
	u8 *pKernel;
	i64 kernelSize;
	i64 pagesOfRam;

	i64 pagesOfPMMStack = pagesOfRam / 512; // 4096(pagesize) / 8(pointerSize)
	struct RegionListCursor cursor = {0};

	i64 pagesForMapTables = 0;

	// identity map the PMM Stack
	for (int i = 0; i < pagesOfPMMStack; i++)
	{
		if (cursor.totalPagesUsed > pagesForMapTables - 10)
			// oh no, we will be short on identity mapped pages, we need to map more
			// lets create a new PT and fill it, so we get another 2MiB to use
			for (int i = 0; i < 512; i++)
				identityMap(pagesForMapTables++);
	}
}

EFI_STATUS
efi_main(EFI_HANDLE imageHandle, EFI_SYSTEM_TABLE *systemTable)
{
	EFI_STATUS result;

	// check that printing works
	result = printlnCon();
	successCheck(result);

	result = initScreen(systemTable);
	successCheck(result);

	result = initMmapAndExit(systemTable, imageHandle);
	successCheck(result);

	drawBackground();
	print("Hello world");
	println();
	print("WE IN GRAPHICS MODE!!");
	println();
	print("and we've exited bootservices");
	println();
	printScreenInfo();
	printMmapInfo();

	while (1)
		__asm__("hlt");
}
