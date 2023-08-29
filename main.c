#include <efi.h>
#include <stdint.h>
 
void* memset(void* bufptr, int value, size_t size) {
	unsigned char* buf = (unsigned char*) bufptr;
	for (size_t i = 0; i < size; i++)
		buf[i] = (unsigned char) value;
	return bufptr;
}

size_t strlen(const uint16_t* str) {
	size_t len = 0;
	while (str[len])
		len++;
	return len;
}

uint16_t * itoa( int value, uint16_t * str, int base )
{
    uint16_t * rc;
    uint16_t * ptr;
    uint16_t * low;
    // Check for supported base.
    if ( base < 2 || base > 36 )
    {
        *str = '\0';
        return str;
    }
    rc = ptr = str;
    // Set '-' for negative decimals.
    if ( value < 0 && base == 10 )
    {
        *ptr++ = '-';
    }
    // Remember where the numbers start.
    low = ptr;
    // The actual conversion.
    do
    {
        // Modulo is negative for negative value. This trick makes abs() unnecessary.
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35 + value % base];
        value /= base;
    } while ( value );
    // Terminating the string.
    *ptr-- = '\0';
    // Invert the numbers.
    while ( low < ptr )
    {
        uint16_t tmp = *low;
        *low++ = *ptr;
        *ptr-- = tmp;
    }
    return rc;
}

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    for (int i = 0;; i++) { 
        uint16_t str[100] = {0};
        itoa(i, str, 10);
        str[strlen(str)] = '\r';
        str[strlen(str)] = '\n';
        SystemTable->ConOut->OutputString(SystemTable->ConOut, str);
        __asm__ hlt
    }
}
