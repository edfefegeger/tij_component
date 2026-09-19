#ifndef __TYPES_H__
#define __TYPES_H__

/*
 * Типы, которыми компонента обменивается с платформой 1С.
 *
 * ВАЖНО: структура tVariant должна в точности повторять официальную из
 * комплекта "Технология создания внешних компонент". Платформа передает
 * параметры методов и принимает результаты через эту структуру по фиксированным
 * смещениям. Любое расхождение (лишний или недостающий член, другой размер поля
 * vt, упаковка структуры) приводит к тому, что компонента читает чужие байты,
 * а платформа неверно понимает возвращаемый результат.
 *
 * Официальная структура (фрагмент types.h из комплекта):
 *
 *     typedef unsigned short TYPEVAR;
 *     struct _tVariant
 *     {
 *         union { ...; struct tm tmVal; ... };   // 40 байт на x64
 *         uint32_t cbElements;
 *         TYPEVAR  vt;                           // 16 бит, а не 32
 *     };                                          // 48 байт на x64
 *
 * Здесь объединение повторено без struct tm и IID (они тянут за собой time.h и
 * типы Windows), но с явным выравнивающим массивом того же размера: важно,
 * чтобы объединение получилось ровно такого же размера, иначе поедут смещения
 * полей cbElements и vt.
 */

#include <stdint.h>

#ifdef _WIN32
    #define ADDIN_API __stdcall
    typedef unsigned short WCHAR_T;   /* 2 байта, как wchar_t в Windows */
#else
    #define ADDIN_API
    typedef wchar_t WCHAR_T;
#endif

/* Тип значения tVariant у платформы: 16-битное целое. */
typedef unsigned short TYPEVAR;

/* Коды типов значений, как в официальном types.h. */
#define VTYPE_EMPTY      0
#define VTYPE_NULL       1
#define VTYPE_I2         2
#define VTYPE_I4         3
#define VTYPE_R4         4
#define VTYPE_R8         5
#define VTYPE_DATE       6
#define VTYPE_TM         7
#define VTYPE_PSTR       8
#define VTYPE_INTERFACE  9
#define VTYPE_ERROR      10
#define VTYPE_BOOL       11
#define VTYPE_VARIANT    12
#define VTYPE_I1         13
#define VTYPE_UI1        14
#define VTYPE_UI2        15
#define VTYPE_UI4        16
#define VTYPE_I8         17
#define VTYPE_UI8        18
#define VTYPE_INT        19
#define VTYPE_UINT       20
#define VTYPE_HRESULT    21
#define VTYPE_PWSTR      22
#define VTYPE_BLOB       23
#define VTYPE_CLSID      24

struct _tVariant
{
    union
    {
        int8_t   val_i8;
        int16_t  val_i16;
        int32_t  val_i32;
        int64_t  val_i64;
        uint8_t  val_ui8;
        uint16_t val_ui16;
        uint32_t val_ui32;
        uint64_t val_ui64;
        bool     val_bool;
        float    val_flt;
        double   val_dbl;

        struct
        {
            char*    pstrVal;
            uint32_t strLen;    /* длина в байтах */
        };

        struct
        {
            WCHAR_T* pwstrVal;
            uint32_t wstrLen;   /* длина в символах */
        };

        /*
         * Выравнивающий массив. В официальной структуре размер объединения
         * задает член struct tm (36 байт), здесь он повторен явно. Убирать
         * нельзя: без него объединение станет короче, и платформа будет
         * читать cbElements и vt не там, где мы их пишем.
         */
        uint8_t  sizeAlign[36];
    };

    uint32_t cbElements;    /* размерность для одномерного массива в pvarVal */
    TYPEVAR  vt;            /* тип значения */
};

typedef struct _tVariant tVariant;

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

/* Перечисление AppCapabilities объявлено в ComponentBase.h, как в официальном
 * комплекте поставки. Здесь его дублировать нельзя. */

#endif /* __TYPES_H__ */
