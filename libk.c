#include "libk.h"
#include "libc.h"

i64 wstrlen(const wchar_t *str)
{
    i64 len = 0;
    while (str[len])
        len++;
    return len;
}

i64 strlen(const char *str)
{
    i64 len = 0;
    while (str[len])
        len++;
    return len;
}

void witoa_buf(i64 value, wchar_t *str, i64 base)
{
    wchar_t *rc;
    wchar_t *ptr;
    wchar_t *low;
    // Check for supported base.
    if (base < 2 || base > 36)
    {
        wchar_t msg[] = L"unsupported base";
        memcpy((void *)str, (void *)msg, sizeof(msg));
        return;
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
}

void itoa_buf(i64 value, char *str, i64 base)
{
    char *rc;
    char *ptr;
    char *low;
    // Check for supported base.
    if (base < 2 || base > 36)
    {
        char msg[] = "unsupported base";
        memcpy((void *)str, (void *)msg, sizeof(msg));
        return;
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
        char tmp = *low;
        *low++ = *ptr;
        *ptr-- = tmp;
    }
}

wchar_t *witoa_base(i64 value, i64 base)
{
    static wchar_t buf[100];
    memset(buf, 0, sizeof(buf));
    witoa_buf(value, buf, base);
    return buf;
}

char *itoa_base(i64 value, i64 base)
{
    static char buf[100];
    memset(buf, 0, sizeof(buf));
    itoa_buf(value, buf, base);
    return buf;
}

wchar_t *witoa(i64 value)
{
    return witoa_base(value, 10);
}

char *itoa(i64 value)
{
    return itoa_base(value, 10);
}
