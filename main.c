#include <efi.h>
#include <stdbool.h>
#include "libk.h"

#define errorCheck(actual, expected) \
	if (actual != expected)          \
	return actual
#define successCheck(actual)   \
	if (actual != EFI_SUCCESS) \
	return actual
#define print(s) systemTable->ConOut->OutputString(systemTable->ConOut, s)
#define println() systemTable->ConOut->OutputString(systemTable->ConOut, L"\r\n")

wchar_t *EFI_MEMORY_TYPE_STRINGS[] = {
	L"EfiReservedMemoryType",
	L"EfiLoaderCode",
	L"EfiLoaderData",
	L"EfiBootServicesCode",
	L"EfiBootServicesData",
	L"EfiRuntimeServicesCode",
	L"EfiRuntimeServicesData",
	L"EfiConventionalMemory",
	L"EfiUnusableMemory",
	L"EfiACPIReclaimMemory",
	L"EfiACPIMemoryNVS",
	L"EfiMemoryMappedIO",
	L"EfiMemoryMappedIOPortSpace",
	L"EfiPalCode",
	L"EfiPersistentMemory",
};

wchar_t *EFI_MEMORY_TYPE_STRINGS_2[] = {
	L"AddressRangeReserved",
	L"AddressRangeMemory",
	L"AddressRangeMemory",
	L"AddressRangeMemory",
	L"AddressRangeMemory",
	L"AddressRangeReserved",
	L"AddressRangeReserved",
	L"AddressRangeMemory",
	L"AddressRangeReserved",
	L"AddressRangeACPI",
	L"AddressRangeNVS",
	L"AddressRangeReserved",
	L"AddressRangeReserved",
	L"AddressRangeReserved",
	L"AddressRangePersistentMemory",
};

bool isUsableMem(UINT32 descType)
{
	return descType == 1 ||
		   descType == 2 ||
		   descType == 3 ||
		   descType == 4 ||
		   descType == 7;
}

EFI_STATUS efi_main(EFI_HANDLE imageHandle, EFI_SYSTEM_TABLE *systemTable)
{
	EFI_STATUS result;

	// check if printing works
	result = println();
	successCheck(result);

	for (int i = 0; i < 100; i++)
	{
		print(itoa(i));
		print(L" ");
		__asm__("hlt");
	}
	println();

	{
		EFI_GUID gopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
		EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
		result = systemTable->BootServices->LocateProtocol(&gopGuid, NULL, (void **)&gop);
		successCheck(result);
		print(L"Got graphics output protocol, listing video modes...\r\n");

		for (int i = 0; i < gop->Mode->MaxMode; i++)
		{
			EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;
			UINTN infoSize;

			result = gop->QueryMode(gop, i, &infoSize, &info);
			print(itoa(i));
			print(L": ");
			print(itoa(info->HorizontalResolution));
			print(L"x");
			print(itoa(info->VerticalResolution));
			print(i == gop->Mode->Mode ? L" (current)\r\n" : L"\r\n");
		}
	}

	{

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
		print(L"Got memory map, listing regions...\r\n");

		int n = mapSize / descriptorSize;
		int usableMemPartSize = 0;
		int usableMemTotalSize = 0;
		for (int i = 0; i < n; i++)
		{
			EFI_MEMORY_DESCRIPTOR *desc = (EFI_MEMORY_DESCRIPTOR *)((uint64_t)memoryMap + (i * descriptorSize));

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
					print(itoa(i - 1));
					print(L": AddressRangeMemory - ");
					print(itoa(usableMemPartSize));
					print(L" KiB\r\n");
					usableMemPartSize = 0;
				}
				print(itoa(i));
				print(L": ");
				print(EFI_MEMORY_TYPE_STRINGS[desc->Type]);
				print(L" - ");
				print(itoa(desc->NumberOfPages * 4));
				print(L" KiB\r\n");
			}
		}
		print(L"Total usable mem size ");
		print(itoa(usableMemTotalSize));
		print(L" KiB\r\n");
		// example usable mem: 7 808 Kib, 3 129 268 Kib, 3 428 KiB, 7 340 032 KiB

		print(L"Leaving boot services!\r\n");
		// https://uefi.org/specs/UEFI/2.9_A/07_Services_Boot_Services.html#efi-boot-services-exitbootservices
		// https://forum.osdev.org/viewtopic.php?f=1&t=42288
		// the first call might not work
		while ((result = systemTable->BootServices->ExitBootServices(imageHandle, mapKey)) == EFI_INVALID_PARAMETER)
		{
			mapSize = bufSize;
			result = systemTable->BootServices->GetMemoryMap(&mapSize, memoryMap, &mapKey, &descriptorSize, &descriptorVersion);
			successCheck(result);
		}
		// causes a kvm internal error
		// print(L"Left boot services!\r\n");
	}

	while (1)
		__asm__("hlt");
}
