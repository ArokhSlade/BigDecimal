#include "G_Essentials.h"
const uint32 SignMask32         = 0x80000000;
const uint32 MantissaMask32     = 0x7fffff;
const uint32 ExpMask32          = 0x7f800000;
const int32  FloatBias32        = 127;

constexpr real32 Max32 = 0x1.fffffep+127f;
constexpr real32 Min32 = 0x1p-149f;

const real32 Inf32 = 0x1p+128f;
const u32 NaN32Bits = 0x7fb00001; //quiet nan with payload "1"
#define NaN32 (*((f32*)&NaN32Bits))

const uint64 SignMask64         = 0x8000'0000'0000'0000;
const uint64 MantissaMask64     = 0x000f'ffff'ffff'ffff;
const uint64 ExpMask64          = 0x7ff0'0000'0000'0000;
const int64  FloatBias64        = 1023;

constexpr real64 Max64 = 0x1.f'ffff'ffff'ffffep+1023f;
constexpr real64 Min64 = 0x1p-1075f;

const f64 Inf64 = 0x1p+1024f;
const u64 NaN64Bits = 0x7ff8'0000'0000'0001; //quiet nan with payload "1"
#define NaN64 (*((f64*)&NaN64Bits))

constexpr int32 DOUBLE_PRECISION = 53;
constexpr int32 FLOAT_PRECISION = 24;

const int64 MinExponent64       = -FloatBias64 - DOUBLE_PRECISION + 1;


//todo 2022 02 05: remove old implementation, then enable this
//inline bool32 IsNegative(real32 Number)
//{
//    return Number & SignMask32;
//}

inline bool32 IsNan(real32 Number)
{
    return Number != Number;
}

inline bool32 IsNAN(real32 Number) {
    return IsNan(Number);
}

inline bool32 IsInf(real32 Number)
{
    return (*(uint32 *)&Number & ~SignMask32) == ExpMask32;
}

inline bool32 IsFinite(real32 Number)
{
    return (*(uint32 *)&Number & ~SignMask32) < ExpMask32;
}

inline real32 Abs(real32 Number)
{
    *(uint32 *)&Number &= ~SignMask32;
    return Number;
}

inline u32 FloatBits(f32 F){
    return *reinterpret_cast<int32 *>(&F);
}

inline int32 Exponent32(real32 F) {
    int32 Result = *reinterpret_cast<int32 *>(&F) & ExpMask32;
    Result >>= 23;
    Result -= FloatBias32;
    return Result;
}

inline bool32 IsDenormalized(real32 Number) {
    return (FloatBits(Number) & ExpMask32) == 0;
}


inline uint32 Mantissa32(real32 F, bool prepend_implicit = false) {
    uint32 Result = *reinterpret_cast<uint32*>(&F) & MantissaMask32;

    if (prepend_implicit && !IsDenormalized(F)) {
        Result |= 0x800000;
    }
    return Result;
}



inline uint32 Mantissa32Explicit(real32 F) {
    return Mantissa32(F,true);
}

inline uint32 Mantissa32Implicit(real32 F) {
    return Mantissa32(F,false);
}


inline i32 TrueExponent32(real32 F){
    i32 Result = Exponent32(F);
    if (IsDenormalized(F)) {
        u32 Mantissa = Mantissa32(F);
        Result -= 22-BitScanReverse32(Mantissa);
    }
    return Result;
}


//-----------------------------------
//DOUBLE
//-----------------------------------
#define IMPLEMENTED_DOUBLE 1
#if IMPLEMENTED_DOUBLE
inline bool32 IsNan(f64 Number)
{
    return Number != Number;
}

inline bool32 IsNAN(f64 Number) {
    return IsNan(Number);
}

inline bool32 IsInf(f64 Number)
{
    return (*(u64 *)&Number & ~SignMask64) == ExpMask64;
}

inline bool32 IsFinite(f64 Number)
{
    return (*(u64 *)&Number & ~SignMask64) < ExpMask64;
}

inline f64 Abs(f64 Number)
{
    *(u64 *)&Number &= ~SignMask64;
    return Number;
}

inline u64 DoubleBits(f64 F){
    return *reinterpret_cast<int64 *>(&F);
}

inline u64 Exponent64(f64 F) {
    i64 Result = *reinterpret_cast<i64 *>(&F) & ExpMask64;
    Result >>= (DOUBLE_PRECISION - 1);
    Result -= FloatBias64;
    return Result;
}


inline u64 FloatBits(f64 F){
    return *reinterpret_cast<u64 *>(&F);
}

inline bool32 IsDenormalized(f64 Number) {
    return (FloatBits(Number) & ExpMask64) == 0;
}


inline u64 Mantissa64(f64 F, bool prepend_implicit = false) {
    u64 Result = *reinterpret_cast<u64*>(&F) & MantissaMask64;

    if (prepend_implicit && !IsDenormalized(F)) {
        Result |= 0x10'0000'0000'0000;
    }
    return Result;
}

inline u64 Mantissa64Explicit(f64 F) {
    return Mantissa64(F,true);
}

inline u64 Mantissa64Implicit(f64 F) {
    return Mantissa64(F,false);
}

inline u64 TrueExponent64(f64 F){
    i32 Result = Exponent64(F);
    if (IsDenormalized(F)) {
        u64 Mantissa = Mantissa64(F);
        Result -= (DOUBLE_PRECISION - 2)-BitScanReverse<u64>(Mantissa);
    }
    return Result;
}
#endif
