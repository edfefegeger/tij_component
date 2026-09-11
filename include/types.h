#ifndef __TYPES_H__
#define __TYPES_H__

#include <stdint.h>

#ifdef _WIN32
    #define ADDIN_API __stdcall
    typedef unsigned short WCHAR_T;
#else
    #define ADDIN_API
    typedef wchar_t WCHAR_T;
#endif

typedef long tVariant_type;

#define VTYPE_EMPTY  0
#define VTYPE_NULL   1
#define VTYPE_I2     2
#define VTYPE_I4     3
#define VTYPE_R4     4
#define VTYPE_R8     5
#define VTYPE_BOOL   11
#define VTYPE_PWSTR  22
#define VTYPE_PSTR   8
#define VTYPE_BLOB   23

#pragma pack(push, 1)
struct tVariant {
    union {
        int8_t      val_i8;
        int16_t     val_i16;
        int32_t     val_i32;
        int64_t     val_i64;
        uint8_t     val_ui8;
        uint16_t    val_ui16;
        uint32_t    val_ui32;
        uint64_t    val_ui64;
        bool        val_bool;
        float       val_flt;
        double      val_dbl;
        struct {
            char*    pstrVal;
            uint32_t strLen;
        };
        struct {
            WCHAR_T* pwstrVal;
            uint32_t wstrLen;
        };
    };
    tVariant_type vt;
    uint32_t      dummy;
};
#pragma pack(pop)

#endif
