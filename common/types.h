#pragma once

/* C23 standard integer types */
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef signed long long int64_t;

typedef unsigned long size_t;
typedef long ssize_t;
typedef unsigned long uintptr_t;
typedef long intptr_t;

/* C23 nullptr constant */
#define nullptr ((void*)0)
#define NULL nullptr

/* C23 bool type - using _Bool directly */
#define bool _Bool
#define true 1
#define false 0

/* C23 static_assert for compile-time checks */
#define static_assert _Static_assert

/* Verify our type sizes at compile time using C23 static_assert */
static_assert(sizeof(uint8_t) == 1, "uint8_t must be 1 byte");
static_assert(sizeof(uint16_t) == 2, "uint16_t must be 2 bytes");
static_assert(sizeof(uint32_t) == 4, "uint32_t must be 4 bytes");
static_assert(sizeof(uint64_t) == 8, "uint64_t must be 8 bytes");
