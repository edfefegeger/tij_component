#ifndef __TYPES_H__
#define __TYPES_H__

/*
 * Локальная замена заголовков из комплекта "Технология внешних компонент" 1С.
 * Если есть доступ к комплекту поставки (ИТС, раздел 3221), предпочтительно
 * использовать официальные заголовки и сверить с ними этот файл.
 */

#include <stdint.h>

#ifdef _WIN32
    #define ADDIN_API __stdcall
    typedef unsigned short WCHAR_T;   /* 2 байта, как WCHAR в 1С */
#else
    #define ADDIN_API
    typedef wchar_t WCHAR_T;
#endif

/*
 * Тип значения в tVariant. На Windows long == 4 байта, но задаём размер явно,
 * чтобы разметка структуры не зависела от компилятора и целевой платформы.
 */
typedef uint32_t tVariant_type;

/* Коды типов значений tVariant, как их определяет платформа 1С:Предприятие. */
#define VTYPE_EMPTY  0
#define VTYPE_NULL   1
#define VTYPE_I2     2
#define VTYPE_I4     3
#define VTYPE_R4     4
#define VTYPE_R8     5
#define VTYPE_PSTR   8
#define VTYPE_BOOL   11
#define VTYPE_PWSTR  22
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

/* --- Перечисления платформы, используемые в экспортируемых функциях --- */

/*
 * Возможности платформы, о поддержке которых компонента сообщает
 * в SetPlatformCapabilities().
 */
typedef enum {
    eAppCapabilitiesInvalid = -1,
    eAppCapabilities1       = 1,
    eAppCapabilitiesLast    = eAppCapabilities1
} AppCapabilities;

/*
 * Тип подключения, который поддерживает компонента. Возвращается из
 * GetAttachType(). Если функция не экспортируется, платформа выбирает тип
 * подключения по режиму совместимости конфигурации.
 */
typedef enum {
    eCanAttachNotIsolated = 1,  /* подключение к процессу платформы */
    eCanAttachIsolated    = 2,  /* подключение к отдельному хост-процессу */
    eCanAttachAny         = 3   /* любое подключение */
} AttachType;

#endif /* __TYPES_H__ */
