#pragma once

#include <stdio.h>

#define PRINTHEX8ARRAY(array, length) \
for(size_t i = 0; i < length; i++) \
	printf("%02X", ((unsigned char*) array)[i]); \
printf("\r\n");
