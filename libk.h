#pragma once
#include "def.h"
#include <stddef.h>

i64 wstrlen(const wchar_t *str);
void witoa_buf(i64 value, wchar_t *str, i64 base);
wchar_t *witoa_base(i64 value, i64 base);
wchar_t *witoa(i64 value);

i64 strlen(const char *str);
void itoa_buf(i64 value, char *str, i64 base);
char *itoa_base(i64 value, i64 base);
char *itoa(i64 value);
