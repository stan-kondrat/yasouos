#pragma once

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

#define NULL ((void*)0)

/* Use _Bool from C23 standard */
#if __STDC_VERSION__ >= 202311L
    #define bool _Bool
    #define true 1
    #define false 0
#else
    typedef int bool;
    #define true 1
    #define false 0
#endif