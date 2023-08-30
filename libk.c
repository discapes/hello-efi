#include "libk.h"
#include "libc.h"

size_t strlen(const wchar_t *str)
{
    size_t len = 0;
    while (str[len])
        len++;
    return len;
}

wchar_t *itoa_buf(int value, wchar_t *str, int base)
{
    wchar_t *rc;
    wchar_t *ptr;
    wchar_t *low;
    // Check for supported base.
    if (base < 2 || base > 36)
    {
        *str = '\0';
        return str;
    }
    rc = ptr = str;
    // Set '-' for negative decimals.
    if (value < 0 && base == 10)
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
    } while (value);
    // Terminating the string.
    *ptr-- = '\0';
    // Invert the numbers.
    while (low < ptr)
    {
        wchar_t tmp = *low;
        *low++ = *ptr;
        *ptr-- = tmp;
    }
    return rc;
}

wchar_t *itoa(int value)
{
    static wchar_t buf[100];
    memset(buf, 0, sizeof(buf));
    itoa_buf(value, buf, 10);
    return buf;
}