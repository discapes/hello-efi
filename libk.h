#pragma once
#include <stddef.h>

size_t strlen(const wchar_t* str);
wchar_t * itoa_buf(int value, wchar_t * str, int base );
wchar_t * itoa(int value);