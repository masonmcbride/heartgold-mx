#ifndef HEARTGOLD_MX_NITRO_TYPES_H
#define HEARTGOLD_MX_NITRO_TYPES_H

/* Locally authored DS scalar compatibility interface, not a Nintendo SDK file.
 * LP64 macOS long/pointer sizes must never determine a DS serialized width.
 * This header provides no hardware access or pointer/address conversion APIs.
 */
#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
/* Original ARM9/CW data ABI aligns 64-bit integers to four bytes. */
typedef uint64_t u64 __attribute__((aligned(4)));
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64 __attribute__((aligned(4)));
typedef float f32;
typedef double f64;

typedef volatile u8 vu8;
typedef volatile u16 vu16;
typedef volatile u32 vu32;
typedef volatile u64 vu64;
typedef volatile s8 vs8;
typedef volatile s16 vs16;
typedef volatile s32 vs32;
typedef volatile s64 vs64;

typedef int32_t BOOL;
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#endif
