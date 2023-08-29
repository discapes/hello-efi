#include <efi.h>
#include "libk.h"

#define errorCheck(actual, expected) if(actual != expected) return actual
#define successCheck(actual) if(actual != EFI_SUCCESS) return actual
#define print(s) systemTable->ConOut->OutputString(systemTable->ConOut, s)
#define println() systemTable->ConOut->OutputString(systemTable->ConOut, L"\r\n")

wchar_t* EFI_MEMORY_TYPE_STRINGS[] = {
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


wchar_t* EFI_MEMORY_TYPE_STRINGS_2[] = {
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

EFI_STATUS efi_main(EFI_HANDLE imageHandle, EFI_SYSTEM_TABLE *systemTable)
{
    EFI_STATUS result;

    for (int i = 0; i < 100; i++) { 
        wchar_t str[100] = {0};
        itoa(i, str, 10);
        str[strlen(str)] = ' ';
        result = print(str);
        successCheck(result);
        __asm__("hlt");
    }
    println();
    
    UINTN mapSize = 0;
    UINTN mapKey;
    UINTN descriptorSize;
    UINT32 descriptorVersion;
    EFI_MEMORY_DESCRIPTOR *memoryMap;

    result = systemTable->BootServices->GetMemoryMap(&mapSize, memoryMap, &mapKey, &descriptorSize, &descriptorVersion);
    errorCheck(result, EFI_BUFFER_TOO_SMALL);

    mapSize += 2 * descriptorSize;
    UINTN buzSize = mapSize;
    result = systemTable->BootServices->AllocatePool(EfiLoaderData, mapSize, (void**)&memoryMap);
    successCheck(result);

    result = systemTable->BootServices->GetMemoryMap(&mapSize, memoryMap, &mapKey, &descriptorSize, &descriptorVersion);
    successCheck(result);

    int n = mapSize / descriptorSize;
    for (int i = 0; i < n; i++) {
        EFI_MEMORY_DESCRIPTOR *desc = (EFI_MEMORY_DESCRIPTOR*)((uint64_t)memoryMap + (i*descriptorSize));
        wchar_t nPagesStr[100] = {0};
        itoa(desc->NumberOfPages, nPagesStr, 10);

        print(EFI_MEMORY_TYPE_STRINGS_2[desc->Type]);
        print(L" - ");
        print(nPagesStr);
        println();
    }

    print(L"Leaving boot services!\r\n"); 
    // https://uefi.org/specs/UEFI/2.9_A/07_Services_Boot_Services.html#efi-boot-services-exitbootservices
    // https://forum.osdev.org/viewtopic.php?f=1&t=42288    
    // the first call might not work
    while ((result = systemTable->BootServices->ExitBootServices(imageHandle, mapKey)) == EFI_INVALID_PARAMETER)
    {
        mapSize = buzSize;
        result = systemTable->BootServices->GetMemoryMap(&mapSize, memoryMap, &mapKey, &descriptorSize, &descriptorVersion);
        successCheck(result);
    }
    // causes a kvm internal error
   // print(L"Left boot services!\r\n");

    while (1) __asm__("hlt");
}
