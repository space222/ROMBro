#pragma once
#include <cstdint>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;

typedef int64_t s64;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t   s8;

union reg
{
	struct {
		u8 l, h;	
	} b;
	u16 w;
};

enum mapper_id { NO_MBC, MBC1, MBC2, MBC3, MBC5, MBC6, MBC7 };
enum exram_type { NO_RAM, KB2, BANKS1, BANKS4, BANKS16, BANKS8, MBC2_RAM }; 

