#ifndef INTRINSICS_H
#define INTRINSICS_H

#include <cmath>

inline bool32 IsNegative(real32 x){
    return std::signbit(x);
}

inline real32 SquareRoot(real32 x) {
    HardAssert(x >= 0.f);
	return sqrtf(x);
}

inline int32
RoundReal32ToInt32(real32 Real32)
{
	int32 Result = (int32)roundf(Real32);
	return(Result);
}

inline uint32
RoundReal32ToUInt32(real32 Real32)
{
	uint32 Result = (uint32)roundf(Real32);
	return(Result);
}

inline int32
FloorReal32ToInt32(real32 Real32)
{
	int32 Result = (int32)floorf(Real32);
	return(Result);
}

inline int32
TruncateReal32ToInt32(real32 Real32)
{
	int32 Result = (int32)Real32;
	return(Result);
}

inline int32
CeilReal32ToInt32(real32 Real32)
{
    int32 Result = (int32)ceilf(Real32);
    return Result;
}

inline real32
Sin(real32 Angle)
{
	real32 Result = sinf(Angle);
	return(Result);
}

inline real32
Cos(real32 Angle)
{
	real32 Result = cosf(Angle);
	return(Result);
}

inline real32
ATan2(real32 Y, real32 X)
{
	real32 Result = atan2f(Y, X);
	return(Result);
}

struct bit_scan_result
{
	bool32 Found;
	uint32 Index;
};

///returns 1 for -, 0 for +
inline bool32 Sign(real32 A)
{
    return *(reinterpret_cast<uint32*>(&A))&(1<<31) && true;
}

inline bool32 Sign(f64 A)
{
    return *(reinterpret_cast<uint64*>(&A))&(1<<63) && true;
}

inline bool32 Sign(int32 I) {
    return *(reinterpret_cast<uint32*>(&I))&(1<<31) && true;
}

inline real32 SignOne(real32 A)
{
    return A< 0.f? -1.f : 1.f;
}

inline bit_scan_result
FindLeastSignificantSetBit(uint32 Value)
{
	bit_scan_result Result = {};

#if COMPILER_MSVC
	Result.Found = _BitScanForward((unsigned long *)&Result.Index, Value);
#else
	for (uint32 Test = 0;
		Test < 32;
		++Test)
	{
		if (Value & (1 << Test))
		{
			Result.Index = Test;
			Result.Found = true;
			break;
		}
	}
#endif

	return(Result);
}


#endif //INTRINSICS_H
